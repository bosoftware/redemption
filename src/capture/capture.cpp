/*
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Product name: redemption, a FLOSS RDP proxy
   Copyright (C) Wallix 2013
   Author(s): Christophe Grosjean, Javier Caverni, Xavier Dunat,
              Martin Potier, Jonatan Poelen, Raphael Zhou, Meng Tan,
              Jennifer Inthavong
*/

#include <algorithm>
#include <chrono>
#include <utility>
#include <string>

#include <ctime> // localtime_r
#include <cstdio> //snprintf
#include <cstdlib> //mkostemps
#include <cerrno>
#include <cassert>

#include "core/error.hpp"

#include "utils/log.hpp"

#include "utils/sugar/algostring.hpp"
#include "utils/sugar/array_view.hpp"
#include "utils/sugar/unique_fd.hpp"
#include "utils/sugar/byte.hpp"
#include "utils/sugar/not_null_ptr.hpp"
#include "utils/sugar/noncopyable.hpp"
#include "utils/sugar/cast.hpp"
#include "utils/sugar/make_unique.hpp"

#include "utils/bitmap_shrink.hpp"
#include "utils/colors.hpp"
#include "utils/difftimeval.hpp"
#include "utils/fileutils.hpp"
#include "utils/key_qvalue_pairs.hpp"
#include "utils/png.hpp"
#include "utils/stream.hpp"
#include "utils/timestamp_tracer.hpp"

#include "transport/out_file_transport.hpp"
#include "transport/out_filename_sequence_transport.hpp"

#include "gdi/capture_probe_api.hpp"
#include "gdi/capture_api.hpp"
#include "gdi/kbd_input_api.hpp"

#include "capture/title_extractors/agent_title_extractor.hpp"
#include "capture/title_extractors/ocr_title_filter.hpp"
#include "capture/title_extractors/ocr_titles_extractor.hpp"
#include "capture/title_extractors/ppocr_titles_extractor.hpp"
#include "capture/title_extractors/ocr_title_extractor_builder.hpp"

#include "capture/capture.hpp"
#include "capture/wrm_capture.hpp"
#include "capture/utils/match_finder.hpp"

#include "capture/video_capture.hpp"

#include "utils/recording_progress.hpp"
#include "utils/sugar/underlying_cast.hpp"


class PatternSearcher
{
    struct TextSearcher
    {
        re::Regex::PartOfText searcher;
        re::Regex::range_matches matches;

        void reset(re::Regex & rgx) {
            this->searcher = rgx.part_of_text_search(false);
        }

        bool next(uint8_t const * uchar) {
            return re::Regex::match_success == this->searcher.next(char_ptr_cast(uchar));
        }

        re::Regex::range_matches const & match_result(re::Regex & rgx) {
            this->matches.clear();
            return rgx.match_result(this->matches, false);
        }
    };

    class Utf8KbdData
    {
        static constexpr const size_t buf_len = 128;

        uint8_t kbd_data[buf_len] = { 0 };
        uint8_t * p = kbd_data;
        uint8_t * beg = p;

        uint8_t * data_begin() {
            using std::begin;
            return begin(this->kbd_data);
        }
        uint8_t * data_end() {
            using std::end;
            return end(this->kbd_data);
        }

    public:
        uint8_t const * get_data() const {
            return this->beg;
        }

        void reset() {
            this->p = this->kbd_data;
            this->beg = this->p;
        }

        void push_utf8_char(uint8_t const * c, size_t char_len) {
            assert(c && char_len <= 4);

            if (static_cast<size_t>(this->data_end() - this->beg) < char_len + 1u) {
                std::size_t pchar_len = 0;
                do {
                    size_t const len = get_utf8_char_size(this->beg);
                    size_t const tailroom = this->data_end() - this->beg;
                    if (tailroom < len) {
                        this->beg = this->data_begin() + (len - tailroom);
                    }
                    else {
                        this->beg += len;
                    }
                    pchar_len += len;
                } while (pchar_len < char_len + 1);
            }

            auto ec = c + char_len;
            for (; c != ec; ++c) {
                *this->p = *c;
                ++this->p;
                if (this->p == this->data_end()) {
                    this->p = this->data_begin();
                }
            }
            *this->p = 0;
        }

        void linearize() {
            if (!this->is_linearized()) {
                std::rotate(this->data_begin(), this->beg, this->data_end());
                auto const diff = this->beg - this->p;
                this->p = this->data_end() - diff;
                this->beg = this->data_begin();
            }
        }

        bool is_linearized() const {
            return this->beg <= this->p;
        }
    };

    utils::MatchFinder::NamedRegexArray regexes_filter;
    std::unique_ptr<TextSearcher[]> regexes_searcher;
    Utf8KbdData utf8_kbd_data;

public:
    PatternSearcher(utils::MatchFinder::ConfigureRegexes conf_regex, char const * filters, int verbose = 0) {
        utils::MatchFinder::configure_regexes(conf_regex, filters, this->regexes_filter, verbose, true);
        auto const count_regex = this->regexes_filter.size();
        if (count_regex) {
            this->regexes_searcher = std::make_unique<TextSearcher[]>(count_regex);
            auto searcher_it = this->regexes_searcher.get();
            for (auto & named_regex : this->regexes_filter) {
                searcher_it->reset(named_regex.regex);
                ++searcher_it;
            }
        }
    }

    void rewind_search() {
        TextSearcher * test_searcher_it = this->regexes_searcher.get();
        for (utils::MatchFinder::NamedRegex & named_regex : this->regexes_filter) {
            test_searcher_it->reset(named_regex.regex);
            ++test_searcher_it;
        }
    }

    template<class Report>
    bool test_uchar(uint8_t const * const utf8_char, size_t const char_len, Report report)
    {
        if (char_len == 0) {
            return false;
        }

        bool has_notify = false;

        utf8_kbd_data.push_utf8_char(utf8_char, char_len);
        TextSearcher * test_searcher_it = this->regexes_searcher.get();

        for (utils::MatchFinder::NamedRegex & named_regex : this->regexes_filter) {
            if (test_searcher_it->next(utf8_char)) {
                utf8_kbd_data.linearize();
                char const * char_kbd_data = ::char_ptr_cast(utf8_kbd_data.get_data());
                if (named_regex.regex.search_with_matches(char_kbd_data)) {
                    auto & match_result = test_searcher_it->match_result(named_regex.regex);
                    auto str = (!match_result.empty() && match_result[0].first)
                        ? match_result[0].first
                        : char_kbd_data;
                    report(named_regex.name.c_str(), str);
                    has_notify = true;
                }
                test_searcher_it->reset(named_regex.regex);
            }

            ++test_searcher_it;
        }
        if (has_notify) {
            utf8_kbd_data.reset();
        }

        return has_notify;
    }

    template<class Report>
    bool test_uchar(uint32_t uchar, Report report)
    {
        uint8_t utf8_char[5];
        size_t const char_len = UTF32toUTF8(uchar, utf8_char, 4u);
        return this->test_uchar(utf8_char, char_len, report);
    }

    bool is_empty() const {
        return this->regexes_filter.empty();
    }
};


enum class FilteringSlash{ No, Yes };
using filter_slash = std::integral_constant<FilteringSlash, FilteringSlash::Yes>;
using nofilter_slash = std::integral_constant<FilteringSlash, FilteringSlash::No>;
template<class Utf8CharFn, class NoPrintableFn, class FilterSlash>
void filtering_kbd_input(uint32_t uchar, Utf8CharFn utf32_char_fn,
                         NoPrintableFn no_printable_fn, FilterSlash filter_slash)
{
    switch (uchar)
    {
        case '/':
            if (filter_slash == FilteringSlash::Yes) {
                no_printable_fn(cstr_array_view("//"));
            }
            else {
                utf32_char_fn(uchar);
            }
            break;
        #define Case(i, s) case i: no_printable_fn(cstr_array_view(s)); break
        Case(0x00000008, "/<backspace>");
        Case(0x00000009, "/<tab>");
        Case(0x0000000D, "/<enter>");
        Case(0x0000001B, "/<escape>");
        Case(0x0000007F, "/<delete>");
        Case(0x00002190, "/<left>");
        Case(0x00002191, "/<up>");
        Case(0x00002192, "/<right>");
        Case(0x00002193, "/<down>");
        Case(0x00002196, "/<home>");
        Case(0x00002198, "/<end>");
        #undef Case
        default: utf32_char_fn(uchar);
    }
}


class PatternKbd final : public gdi::KbdInputApi
{
    ReportMessageApi * report_message;
    PatternSearcher pattern_kill;
    PatternSearcher pattern_notify;

public:
    PatternKbd(
        ReportMessageApi * report_message,
        char const * str_pattern_kill, char const * str_pattern_notify,
        int verbose = 0)
    : report_message(report_message)
    , pattern_kill(utils::MatchFinder::ConfigureRegexes::KBD_INPUT,
                   str_pattern_kill && report_message ? str_pattern_kill : nullptr, verbose)
    , pattern_notify(utils::MatchFinder::ConfigureRegexes::KBD_INPUT,
                     str_pattern_notify && report_message ? str_pattern_notify : nullptr, verbose)
    {}

    bool contains_pattern() const {
        return !this->pattern_kill.is_empty() || !this->pattern_notify.is_empty();
    }

    bool kbd_input(const timeval& /*now*/, uint32_t uchar) override {
        bool can_be_sent_to_server = true;

        filtering_kbd_input(
            uchar,
            [this, &can_be_sent_to_server](uint32_t uchar) {
                uint8_t buf_char[5];
                size_t const char_len = UTF32toUTF8(uchar, buf_char, sizeof(buf_char));

                if (char_len > 0) {
                    buf_char[char_len] = '\0';
                    if (!this->pattern_kill.is_empty()) {
                        can_be_sent_to_server &= !this->test_pattern(
                            buf_char, char_len, this->pattern_kill, true
                        );
                    }
                    if (!this->pattern_notify.is_empty()) {
                        this->test_pattern(
                            buf_char, char_len, this->pattern_notify, false
                        );
                    }
                }
            },
            [this](array_view_const_char const &) {
                this->pattern_kill.rewind_search();
                this->pattern_notify.rewind_search();
            },
            nofilter_slash{}
        );

        return can_be_sent_to_server;
    }

    void enable_kbd_input_mask(bool /*enable*/) override {
    }

private:
    KeyQvalueFormatter message;
    bool test_pattern(
        uint8_t const * uchar, size_t char_len,
        PatternSearcher & searcher, bool is_pattern_kill
    ) {
        return searcher.test_uchar(
            uchar, char_len,
            [&, this](std::string const & pattern, char const * str) {
                assert(this->report_message);
                utils::MatchFinder::report(
                    *this->report_message,
                    is_pattern_kill,
                    utils::MatchFinder::ConfigureRegexes::KBD_INPUT,
                    pattern.c_str(),
                    str
                );
            }
        );
    }
};



class SyslogKbd final : public gdi::KbdInputApi, public gdi::CaptureApi
{
    uint8_t kbd_buffer[1024];
    OutStream kbd_stream;
    bool keyboard_input_mask_enabled = false;
    timeval last_snapshot;

private:
    void write_shadow_keys() {
        if (!this->kbd_stream.has_room(1)) {
            this->flush();
        }
        this->kbd_stream.out_uint8('*');
    }

    void write_keys(uint32_t uchar) {
        filtering_kbd_input(
            uchar,
            [this](uint32_t uchar) {
                uint8_t buf_char[5];
                if (size_t const char_len = UTF32toUTF8(uchar, buf_char, sizeof(buf_char))) {
                    this->copy_bytes({buf_char, char_len});
                }
            },
            [this](array_view_const_char no_printable_str) {
                this->copy_bytes(no_printable_str);
            },
            filter_slash{}
        );
    }

    void copy_bytes(const_byte_array bytes) {
        if (this->kbd_stream.tailroom() < bytes.size()) {
            this->flush();
        }
        this->kbd_stream.out_copy_bytes(bytes.data(), std::min(this->kbd_stream.tailroom(), bytes.size()));
    }

public:
    explicit SyslogKbd(timeval const & now)
    : kbd_stream(this->kbd_buffer)
    , last_snapshot(now)
    {}

    ~SyslogKbd() {
        this->flush();
    }

    void enable_kbd_input_mask(bool enable) override {
        if (this->keyboard_input_mask_enabled != enable) {
            this->flush();
            this->keyboard_input_mask_enabled = enable;
        }
    }

    bool kbd_input(const timeval& /*now*/, uint32_t keys) override {
        if (this->keyboard_input_mask_enabled) {
            this->write_shadow_keys();
        }
        else {
            this->write_keys(keys);
        }
        return true;
    }

    void flush() {
        if (this->kbd_stream.get_offset()) {
            LOG(LOG_INFO, R"x(type="KBD input" data="%.*s")x",
                int(this->kbd_stream.get_offset()),
                reinterpret_cast<char const *>(this->kbd_stream.get_data()));
            this->kbd_stream.rewind();
        }
    }

private:
    Microseconds periodic_snapshot(
        const timeval& now, int cursor_x, int cursor_y, bool ignore_frame_in_timeval
    ) override {
        (void)cursor_x;
        (void)cursor_y;
        (void)ignore_frame_in_timeval;
        std::chrono::microseconds const time_to_wait = std::chrono::seconds{2};
        std::chrono::microseconds const diff {difftimeval(now, this->last_snapshot)};

        if (diff < time_to_wait && this->kbd_stream.get_offset() < 8 * sizeof(uint32_t)) {
            return time_to_wait;
        }

        this->flush();
        this->last_snapshot = now;

        return time_to_wait;
    }
};


namespace {
    constexpr array_view_const_char session_log_prefix() { return cstr_array_view("data='"); }
    constexpr array_view_const_char session_log_suffix() { return cstr_array_view("'"); }
}


class SessionLogKbd final : public gdi::KbdInputApi, public gdi::CaptureProbeApi
{
    OutStream kbd_stream;
    bool keyboard_input_mask_enabled = false;
    static const std::size_t buffer_size = 64;
    uint8_t buffer[buffer_size + session_log_prefix().size() + session_log_suffix().size() + 1];
    bool is_probe_enabled_session = false;
    ReportMessageApi & report_message;

    void copy_bytes(const_byte_array bytes) {
        if (this->kbd_stream.tailroom() < bytes.size()) {
            this->flush();
        }
        this->kbd_stream.out_copy_bytes(bytes.data(), std::min(this->kbd_stream.tailroom(), bytes.size()));
    }

    void write_shadow_keys() {
        if (!this->kbd_stream.has_room(1)) {
            this->flush();
        }
        this->kbd_stream.out_uint8('*');
    }

    void write_keys(uint32_t uchar) {
        filtering_kbd_input(
            uchar,
            [this](uint32_t uchar) {
                uint8_t buf_char[5];
                if (size_t const char_len = UTF32toUTF8(uchar, buf_char, sizeof(buf_char))) {
                    this->copy_bytes({buf_char, char_len});
                }
            },
            [this](array_view_const_char no_printable_str) {
                this->copy_bytes(no_printable_str);
            },
            filter_slash{}
        );
    }

public:
    explicit SessionLogKbd(ReportMessageApi & report_message)
    : kbd_stream{this->buffer + session_log_prefix().size(), buffer_size}
    , report_message(report_message)
    {
        memcpy(this->buffer, session_log_prefix().data(), session_log_prefix().size());
    }

    ~SessionLogKbd() {
        this->flush();
    }

    bool kbd_input(const timeval& /*now*/, uint32_t uchar) override {
        if (this->keyboard_input_mask_enabled) {
            if (this->is_probe_enabled_session) {
                this->write_shadow_keys();
            }
        }
        else {
            this->write_keys(uchar);
        }
        return true;
    }

    void enable_kbd_input_mask(bool enable) override {
        if (this->keyboard_input_mask_enabled != enable) {
            this->flush();
            this->keyboard_input_mask_enabled = enable;
        }
    }

private:
    KeyQvalueFormatter formatted_message;

public:
    void flush() {
        if (this->kbd_stream.get_offset()) {
            this->formatted_message.assign("KBD_INPUT", {
                {"data", stream_to_avchar(this->kbd_stream)}
            });

            this->report_message.log5(this->formatted_message.str());

            this->kbd_stream.rewind();
        }
    }

    void session_update(const timeval& /*now*/, array_view_const_char message) override {
        this->is_probe_enabled_session = (::strcasecmp(message.data(), "Probe.Status=Unknown") != 0);
        this->flush();
    }

    void possible_active_window_change() override {
        this->flush();
    }
};




class PatternsChecker : noncopyable
{
    utils::MatchFinder::NamedRegexArray regexes_filter_kill;
    utils::MatchFinder::NamedRegexArray regexes_filter_notify;
    ReportMessageApi & report_message;

public:
    PatternsChecker(ReportMessageApi & report_message, PatternParams const & params)
    : report_message(report_message)
    {
        utils::MatchFinder::configure_regexes(utils::MatchFinder::ConfigureRegexes::OCR,
            params.pattern_kill, this->regexes_filter_kill, params.verbose);

        utils::MatchFinder::configure_regexes(utils::MatchFinder::ConfigureRegexes::OCR,
            params.pattern_notify, this->regexes_filter_notify, params.verbose);
    }

    bool contains_pattern() const {
        return !this->regexes_filter_kill.empty() || !this->regexes_filter_notify.empty();
    }

    void operator()(array_view_const_char str) {
        assert(str.data() && str.size());
        this->check_filter(this->regexes_filter_kill, str.data());
        this->check_filter(this->regexes_filter_notify, str.data());
    }

private:
    void check_filter(utils::MatchFinder::NamedRegexArray & regexes_filter, char const * str) {
        if (regexes_filter.begin()) {
            utils::MatchFinder::NamedRegexArray::iterator first = regexes_filter.begin();
            utils::MatchFinder::NamedRegexArray::iterator last = regexes_filter.end();
            for (; first != last; ++first) {
                if (first->search(str)) {
                    utils::MatchFinder::report(this->report_message,
                        &regexes_filter == &this->regexes_filter_kill, // pattern_kill = FINDPATTERN_KILL
                        utils::MatchFinder::ConfigureRegexes::OCR,
                        first->name.c_str(), str);
                }
            }
        }
    }
};


class PngCapture : public gdi::CaptureApi
{
protected:
    OutFilenameSequenceTransport trans;
    RDPDrawable & drawable;
    timeval start_capture;
    std::chrono::microseconds frame_interval;

private:
    unsigned zoom_factor;
    unsigned scaled_width;
    unsigned scaled_height;

    std::unique_ptr<uint8_t[]> scaled_buffer;

    TimestampTracer timestamp_tracer;

protected:
    gdi::ImageFrameApi & image_frame_api;

    PngCapture(
        const CaptureParams & capture_params, const PngParams & png_params,
        RDPDrawable & drawable,
        gdi::ImageFrameApi & imageFrameApi, MutableImageDataView const & image_view)
    : trans(
        FilenameGenerator::PATH_FILE_COUNT_EXTENSION,
        capture_params.record_tmp_path, capture_params.basename, ".png",
        capture_params.groupid, report_error_from_reporter(capture_params.report_message))
    , drawable(drawable)
    , start_capture(capture_params.now)
    , frame_interval(png_params.png_interval)
    , zoom_factor(png_params.zoom)
    , scaled_width{(((image_view.width() * this->zoom_factor) / 100)+3) & 0xFFC}
    , scaled_height{((image_view.height() * this->zoom_factor) / 100)}
    , timestamp_tracer(image_view)
    , image_frame_api(imageFrameApi)
    {
        if (this->zoom_factor != 100) {
            this->scaled_buffer.reset(new uint8_t[this->scaled_width * this->scaled_height * 3]);
        }
    }

public:
    PngCapture(
        const CaptureParams & capture_params, const PngParams & png_params,
        RDPDrawable & drawable, gdi::ImageFrameApi & imageFrameApi)
    : PngCapture(
        capture_params, png_params, drawable,
        imageFrameApi, imageFrameApi.get_mutable_image_view())
    {}

    void dump(void)
    {
        auto const image_view = this->image_frame_api.get_mutable_image_view();
        if (this->zoom_factor == 100) {
            ::dump_png24(this->trans, image_view, true);
        }
        else {
            scale_data(
                this->scaled_buffer.get(), image_view.data(),
                this->scaled_width, image_view.width(),
                this->scaled_height, image_view.height(),
                image_view.line_size());
            ::dump_png24(
                this->trans, this->scaled_buffer.get(),
                this->scaled_width, this->scaled_height,
                this->scaled_width * 3, false);
        }
    }

     virtual void clear_old() {}

     void clear_png_interval(uint32_t num_start, uint32_t num_end){
        for(uint32_t num = num_start ; num < num_end ; num++) {
            // unlink may fail, for instance if file does not exist, just don't care
            ::unlink(this->trans.seqgen()->get(num));
        }
     }

    Microseconds periodic_snapshot(
        timeval const & now, int x, int y, bool ignore_frame_in_timeval
    ) override {
        (void)x;
        (void)y;
        (void)ignore_frame_in_timeval;
        std::chrono::microseconds const duration = difftimeval(now, this->start_capture);
        std::chrono::microseconds const interval = this->frame_interval;
        if (duration >= interval) {
             // Snapshot at end of Frame or force snapshot if diff_time_val >= 1.5 x frame_interval.
            if (this->drawable.logical_frame_ended() || (duration >= interval * 3 / 2)) {
                this->drawable.trace_mouse();
                tm ptm;
                localtime_r(&now.tv_sec, &ptm);
                this->image_frame_api.prepare_image_frame();
                this->timestamp_tracer.trace(ptm);

                this->dump();
                this->clear_old();
                this->trans.next();

                this->timestamp_tracer.clear();
                this->start_capture = now;
                this->drawable.clear_mouse();

                return interval.count() ? interval - duration % interval : interval;
            }
            else {
                // Wait 0.3 x frame_interval.
                return this->frame_interval / 3;
            }
        }
        return interval - duration;
    }
};

class PngCaptureRT : public PngCapture
{
    uint32_t num_start;
    unsigned png_limit;

    bool enable_rt_display;

public:
    PngCaptureRT(
        const CaptureParams & capture_params, const PngParams & png_params,
        RDPDrawable & drawable, gdi::ImageFrameApi & imageFrameApi)
    : PngCapture(capture_params, png_params, drawable, imageFrameApi)
    , num_start(this->trans.get_seqno())
    , png_limit(png_params.png_limit)
    , enable_rt_display(png_params.rt_display)
    {
    }

    ~PngCaptureRT(){
        this->clear_png_interval(this->num_start, this->trans.get_seqno() + 1);
    }

    void update_config(bool enable_rt_display) {
        if (enable_rt_display != this->enable_rt_display){
            this->enable_rt_display = enable_rt_display;
            // clear files if we go from RT to non-RT
            if (!this->enable_rt_display) {
                this->clear_png_interval(this->num_start, this->trans.get_seqno() + 1);
            }
        }
    }

     void clear_old() override {
        if (this->trans.get_seqno() < this->png_limit) {
            return;
        }
        uint32_t num_start = this->trans.get_seqno() - this->png_limit;
        this->clear_png_interval(num_start, num_start + 1);
    }

    Microseconds periodic_snapshot(
        timeval const & now, int x, int y, bool ignore_frame_in_timeval
    ) override {
        std::chrono::microseconds const duration = difftimeval(now, this->start_capture);
        std::chrono::microseconds const interval = this->frame_interval;
        if (this->enable_rt_display) {
            return this->PngCapture::periodic_snapshot(now, x, y, ignore_frame_in_timeval);
        }
        return interval - duration % interval;
    }

    struct WindowRecord {
        uint32_t window_id;
        uint32_t fields_present_flags;
        uint32_t style;
        uint8_t show_state;
        int32_t visible_offset_x;
        int32_t visible_offset_y;

        WindowRecord(uint32_t window_id, uint32_t fields_present_flags,
                     uint32_t style, uint8_t show_state,
                     int32_t visible_offset_x, int32_t visible_offset_y)
        : window_id(window_id)
        , fields_present_flags(fields_present_flags)
        , style(style)
        , show_state(show_state)
        , visible_offset_x(visible_offset_x)
        , visible_offset_y(visible_offset_y) {}
    };

    std::vector<WindowRecord> windows;

    struct VisibilityRectRecord {
        uint32_t window_id;
        Rect rect;

        VisibilityRectRecord(uint32_t window_id, Rect rect)
        : window_id(window_id)
        , rect(rect) {}
    };

    std::vector<VisibilityRectRecord> windows_rects;

    void new_or_existing_window_event(
            uint32_t window_id, uint32_t fields_present_flags,
            uint32_t style, uint8_t show_state,
            int32_t visible_offset_x, int32_t visible_offset_y,
            std::vector<RDP::RAIL::Rectangle> const & visibility_rects)
            override {
// LOG(LOG_INFO, "PngCaptureRT::new_or_existing_window_event");
        if (fields_present_flags &
            (RDP::RAIL::WINDOW_ORDER_FIELD_STYLE |
             RDP::RAIL::WINDOW_ORDER_FIELD_SHOW |
             RDP::RAIL::WINDOW_ORDER_FIELD_VISOFFSET)) {
            std::vector<WindowRecord>::iterator iter =
                std::find_if(
                        this->windows.begin(),
                        this->windows.end(),
                        [window_id](WindowRecord& window_record) -> bool {
                            return (window_record.window_id == window_id);
                        }
                    );
            if (iter != this->windows.end()) {
                if (fields_present_flags &
                    RDP::RAIL::WINDOW_ORDER_FIELD_STYLE) {
                    iter->style = style;
                    iter->fields_present_flags |= RDP::RAIL::WINDOW_ORDER_FIELD_STYLE;
                }
                if (fields_present_flags &
                    RDP::RAIL::WINDOW_ORDER_FIELD_SHOW) {
                    iter->show_state = show_state;
                    iter->fields_present_flags |= RDP::RAIL::WINDOW_ORDER_FIELD_SHOW;
                }
                if (fields_present_flags &
                    RDP::RAIL::WINDOW_ORDER_FIELD_VISOFFSET) {
                    iter->visible_offset_x = visible_offset_x;
                    iter->visible_offset_y = visible_offset_y;
                    iter->fields_present_flags |= RDP::RAIL::WINDOW_ORDER_FIELD_VISOFFSET;
                }
            }
            else {
                this->windows.emplace_back(window_id,
                    (fields_present_flags &
                     (RDP::RAIL::WINDOW_ORDER_FIELD_STYLE |
                      RDP::RAIL::WINDOW_ORDER_FIELD_SHOW |
                      RDP::RAIL::WINDOW_ORDER_FIELD_VISOFFSET)),
                    style, show_state, visible_offset_x, visible_offset_y);
            }
        }

        if (fields_present_flags & RDP::RAIL::WINDOW_ORDER_FIELD_VISIBILITY) {
            this->windows_rects.erase(
                    std::remove_if(
                        this->windows_rects.begin(),
                        this->windows_rects.end(),
                        [window_id](VisibilityRectRecord& window_rect_record) {
// LOG(LOG_INFO, "remove_if: %s", ((window_rect_record.window_id == window_id) ? "true" : "false"));
                                return (window_rect_record.window_id == window_id);
                            }),
                    this->windows_rects.end()
                );

            if (visibility_rects.size()) {
                for (const RDP::RAIL::Rectangle& rectangle : visibility_rects) {
                    this->windows_rects.emplace_back(
                        window_id,
                        Rect(rectangle.Left(), rectangle.Top(), rectangle.Width(), rectangle.Height()));
                }
            }
        }

        this->update_image_frame_rect();
    }

    void delete_window_event(uint32_t window_id) override {
// LOG(LOG_INFO, "PngCaptureRT::delete_window_event");
        this->windows.erase(
                std::remove_if(this->windows.begin(), this->windows.end(),
                    [window_id](WindowRecord& window_record) {
// LOG(LOG_INFO, "remove_if(1): %s", ((window_record.window_id == window_id) ? "true" : "false"));
                            return (window_record.window_id == window_id);
                        }),
                this->windows.end()
            );

        this->windows_rects.erase(
                std::remove_if(this->windows_rects.begin(),
                    this->windows_rects.end(),
                    [window_id](VisibilityRectRecord& visibility_rect_record) {
// LOG(LOG_INFO, "remove_if(2): %s", ((visibility_rect_record.window_id == window_id) ? "true" : "false"));
                            return (visibility_rect_record.window_id == window_id);
                        }),
                this->windows_rects.end()
            );

        this->update_image_frame_rect();
    }

    void update_image_frame_rect() {
// LOG(LOG_INFO, "");
// LOG(LOG_INFO, "Widnows Rects Size: %zu", this->windows_rects.size());

        Rect new_image_frame_rect;
// new_image_frame_rect.log(LOG_INFO, "] ] ] ] ] NewImageFrameRect");

        std::for_each(
                this->windows.cbegin(),
                this->windows.cend(),
                [&new_image_frame_rect, this](const WindowRecord& window_record) {
                    std::for_each(
                            this->windows_rects.cbegin(),
                            this->windows_rects.cend(),
                            [&new_image_frame_rect, window_record](
                                    const VisibilityRectRecord& visibility_rect_record) {
                                if (window_record.window_id !=
                                    visibility_rect_record.window_id) {
                                    return;
                                }

                                assert(window_record.fields_present_flags &
                                    RDP::RAIL::WINDOW_ORDER_FIELD_VISOFFSET);

// visibility_rect_record.rect.offset(
//     window_record.visible_offset_x, window_record.visible_offset_y).log(
//         LOG_INFO, "} } } } } VisibilityRect");

                                if (!(window_record.style & WS_ICONIC) &&
                                    ((window_record.style & WS_VISIBLE) ||
                                     ((window_record.show_state != SW_FORCEMINIMIZE) &&
                                      (window_record.show_state != SW_HIDE) &&
                                      (window_record.show_state != SW_MINIMIZE)))) {
                                        new_image_frame_rect =
                                            new_image_frame_rect.disjunct(
                                                visibility_rect_record.rect.offset(
                                                        window_record.visible_offset_x,
                                                        window_record.visible_offset_y
                                                    ));
// new_image_frame_rect.log(LOG_INFO, "] ] ] ] ] NewImageFrameRect");
                                }
                            }
                        );
                }
            );

        if (!new_image_frame_rect.isempty()) {
            new_image_frame_rect = new_image_frame_rect.expand(16).intersect(
                {0, 0, this->drawable.width(), this->drawable.height()});


            this->image_frame_api.reset(
                new_image_frame_rect.x, new_image_frame_rect.y,
                new_image_frame_rect.cx, new_image_frame_rect.cy);
        }

// new_image_frame_rect.log(LOG_INFO, "> > > > > NewImageFrameRect");
    }
};


namespace {
    template<std::size_t N>
    inline bool cstr_equal(char const (&s1)[N], array_view_const_char s2) {
        return N - 1 == s2.size() && std::equal(s1, s1 + N - 1, begin(s2));
    }
}

inline void agent_data_extractor(KeyQvalueFormatter & message, array_view_const_char data)
{
    using Av = array_view_const_char;

    auto find = [](Av const & s, char c) {
        auto p = std::find(begin(s), end(s), c);
        return p == end(s) ? nullptr : p;
    };

    auto const pos_separator = find(data, '=');

    message.clear();

    if (pos_separator) {
        auto left = [](Av s, char const * pos) { return Av(begin(s), pos - begin(s)); };
        auto right = [](Av s, char const * pos) { return Av(pos + 1, begin(s) + s.size() - (pos + 1)); };

        auto zstr = [](Av var) {
            // array_view with zero terminal
            return make_array_view(var.data(), var.size()-1);
        };

        auto const order = left(data, pos_separator);
        auto const parameters = (data.back() == '\x0')
          ? Av(pos_separator+1, data.end()-1)
          : Av(pos_separator+1, data.end());

        auto line_with_1_var = [&](Av var1) {
            message.assign(order, {{zstr(var1), parameters}});
        };
        auto line_with_2_var = [&](Av var1, Av var2) {
            if (auto const subitem_separator = find(parameters, '\x01')) {
                message.assign(order, {
                    {zstr(var1), left(parameters, subitem_separator)},
                    {zstr(var2), right(parameters, subitem_separator)},
                });
            }
        };
        auto line_with_3_var = [&](Av var1, Av var2, Av var3) {
            if (auto const subitem_separator = find(parameters, '\x01')) {
                auto const text = left(parameters, subitem_separator);
                auto const remaining = right(parameters, subitem_separator);
                if (auto const subitem_separator2 = find(remaining, '\x01')) {
                    message.assign(order, {
                        {zstr(var1), text},
                        {zstr(var2), left(remaining, subitem_separator2)},
                        {zstr(var3), right(remaining, subitem_separator2)},
                    });
                }
            }
        };
        auto line_with_5_var = [&](Av var1, Av var2, Av var3, Av var4, Av var5) {
            if (auto const subitem_separator = find(parameters, '\x01')) {
                auto const text = left(parameters, subitem_separator);
                auto const remaining = right(parameters, subitem_separator);
                if (auto const subitem_separator2 = find(remaining, '\x01')) {
                    auto const text2 = left(remaining, subitem_separator2);
                    auto const remaining2 = right(remaining, subitem_separator2);
                    if (auto const subitem_separator3 = find(remaining2, '\x01')) {
                        auto const text3 = left(remaining2, subitem_separator3);
                        auto const remaining3 = right(remaining2, subitem_separator3);
                        if (auto const subitem_separator4 = find(remaining3, '\x01')) {
                            auto const text4 = left(remaining3, subitem_separator4);
                            auto const text5 = right(remaining3, subitem_separator4);
                            message.assign(order, {
                                {zstr(var1), text},
                                {zstr(var2), text2},
                                {zstr(var3), text3},
                                {zstr(var4), text4},
                                {zstr(var5), text5},
                            });
                        }
                    }
                }
            }
        };

        // TODO used string_id: switch (sid(order)) { case "string"_sid: ... }
        if (cstr_equal("PASSWORD_TEXT_BOX_GET_FOCUS", order)
         || cstr_equal("UAC_PROMPT_BECOME_VISIBLE", order)) {
            line_with_1_var("status");
        }
        else if (cstr_equal("INPUT_LANGUAGE", order)) {
            line_with_2_var("identifier", "display_name");
        }
        else if (cstr_equal("NEW_PROCESS", order)
              || cstr_equal("COMPLETED_PROCESS", order)) {
            line_with_1_var("command_line");
        }
        else if (cstr_equal("OUTBOUND_CONNECTION_BLOCKED", order)
              || cstr_equal("OUTBOUND_CONNECTION_DETECTED", order)) {
            line_with_2_var("rule", "application_name");
        }
        else if (cstr_equal("FOREGROUND_WINDOW_CHANGED", order)) {
            line_with_3_var("windows", "class", "command_line");
        }
        else if (cstr_equal("BUTTON_CLICKED", order)) {
            line_with_2_var("windows", "button");
        }
        else if (cstr_equal("EDIT_CHANGED", order)) {
            line_with_2_var("windows", "edit");
        }

        else if (cstr_equal("DRIVE_REDIRECTION_USE", order)) {
            line_with_2_var("device_name", "device_type");
        }
        else if (cstr_equal("DRIVE_REDIRECTION_READ", order)
              || cstr_equal("DRIVE_REDIRECTION_WRITE", order)
              || cstr_equal("DRIVE_REDIRECTION_DELETE", order)) {
            line_with_1_var("file_name");
        }
        else if (cstr_equal("DRIVE_REDIRECTION_RENAME", order)) {
            line_with_2_var("old_file_name", "new_file_name");
        }

        else if (cstr_equal("CB_COPYING_PASTING_FILE_TO_REMOTE_SESSION", order)
              || cstr_equal("CB_COPYING_PASTING_FILE_FROM_REMOTE_SESSION", order)) {
            line_with_2_var("file_name", "size");
        }

        else if (cstr_equal("CLIENT_EXECUTE_REMOTEAPP", order)) {
            line_with_1_var("exe_or_file");
        }

        else if (cstr_equal("CERTIFICATE_CHECK_SUCCESS", order)
              || cstr_equal("SERVER_CERTIFICATE_NEW", order)
              || cstr_equal("SERVER_CERTIFICATE_MATCH_SUCCESS", order)
              || cstr_equal("SERVER_CERTIFICATE_MATCH_FAILURE", order)
              || cstr_equal("SERVER_CERTIFICATE_ERROR", order)) {
            line_with_1_var("description");
        }

        else if ((cstr_equal("OUTBOUND_CONNECTION_BLOCKED_2", order)) ||
                 (cstr_equal("OUTBOUND_CONNECTION_DETECTED_2", order))) {
            line_with_5_var("rule", "app_name", "app_cmd_line", "dst_addr", "dst_port");
        }

        else if (cstr_equal("STARTUP_APPLICATION_FAIL_TO_RUN", order)) {
            line_with_2_var("application_name", "raw_result");
        }

        else if (cstr_equal("STARTUP_APPLICATION_FAIL_TO_RUN_2", order)) {
            line_with_3_var("application_name", "raw_result", "raw_result_message");
        }

        else if (cstr_equal("PROCESS_BLOCKED", order)
              || cstr_equal("PROCESS_DETECTED", order)) {
            line_with_3_var("rule", "app_name", "app_cmd_line");
        }

        else {
            message.clear();
            LOG(LOG_WARNING,
                "MetaDataExtractor(): Unexpected order. Data=\"%.*s\"",
                int(data.size()), data.data());
            return ;
        }
    }

    if (!message.av().size()) {
        LOG(LOG_WARNING,
            "MetaDataExtractor(): Invalid data format. Data=\"%.*s\"",
            int(data.size()), data.data());
    }
}

namespace {
    constexpr array_view_const_char session_meta_kbd_prefix() { return cstr_array_view("[Kbd]"); }
    constexpr array_view_const_char session_meta_kbd_suffix() { return cstr_array_view("\n"); }
}

/*
* Format:
*
* $date " - [Kbd]" $kbd
* $date " + " $title
* $date " - " $line
* $date " + " (break)
*
* Info:
*
* + for new video file
*/
class SessionMeta final : public gdi::KbdInputApi, public gdi::CaptureApi, public gdi::CaptureProbeApi
{
    OutStream kbd_stream;
    bool keyboard_input_mask_enabled = false;
    uint8_t kbd_buffer[512];
    static const std::size_t kbd_buffer_usable_char =
        sizeof(kbd_buffer) - session_meta_kbd_prefix().size() - session_meta_kbd_suffix().size();
    uint8_t kbd_chars_size[kbd_buffer_usable_char];
    std::ptrdiff_t kbd_char_pos = 0;
    time_t last_time;
    Transport & trans;
    bool is_probe_enabled_session = false;
    bool previous_char_is_event_flush = false;
    const bool key_markers_hidden_state;

public:
    SessionMeta(const timeval & now, Transport & trans, bool key_markers_hidden_state)
    : kbd_stream{this->kbd_buffer + session_meta_kbd_prefix().size(), kbd_buffer_usable_char}
    , last_time(now.tv_sec)
    , trans(trans)
    , key_markers_hidden_state(key_markers_hidden_state)
    {
        memcpy(this->kbd_buffer, session_meta_kbd_prefix().data(), session_meta_kbd_prefix().size());

        // force file creation even if no text recognized
        this->trans.send("", 0);
    }

    ~SessionMeta() {
        this->send_kbd();
    }

    void enable_kbd_input_mask(bool enable) override {
        if (this->keyboard_input_mask_enabled != enable) {
            this->send_kbd();
            this->keyboard_input_mask_enabled = enable;
        }
    }

    bool kbd_input(const timeval& now, uint32_t uchar) override {
        if (this->keyboard_input_mask_enabled) {
            if (this->is_probe_enabled_session) {
                this->write_shadow_keys();
                this->send_kbd_if_special_char(uchar);
            }
        }
        else {
            this->write_keys(uchar);
            this->send_kbd_if_special_char(uchar);
        }
        this->last_time = now.tv_sec;
        return true;
    }

    void send_line(time_t rawtime, array_view_const_char data) {
        this->send_data(rawtime, data, '-');
    }

    void next_video(time_t rawtime) {
        this->send_data(rawtime, cstr_array_view("(break)"), '+');
    }

private:
    KeyQvalueFormatter formatted_message;

public:
    void title_changed(time_t rawtime, array_view_const_char title) {
        this->formatted_message.assign("TITLE_BAR", {{"data", title}});
        this->send_data(rawtime, this->formatted_message.av(), '+');
    }

    void session_update(const timeval& now, array_view_const_char message) override {
        this->is_probe_enabled_session = (::strcasecmp(message.data(), "Probe.Status=Unknown") != 0);
        agent_data_extractor(this->formatted_message, message);
        if (!this->formatted_message.av().empty()) {
            this->send_line(now.tv_sec, this->formatted_message.av());
        }
    }

    void possible_active_window_change() override {
        this->send_kbd();
        this->previous_char_is_event_flush = true;
    }

    Microseconds periodic_snapshot(
        const timeval& now, int /*cursor_x*/, int /*cursor_y*/, bool /*ignore_frame_in_timeval*/
    ) override {
        this->last_time = now.tv_sec;
        return std::chrono::seconds{10};
    }

private:
    void write_shadow_keys() {
        if (!this->kbd_stream.has_room(1)) {
            this->send_kbd();
        }
        this->kbd_stream.out_uint8('*');
    }

    void write_keys(uint32_t uchar) {
        filtering_kbd_input(
            uchar,
            [this](uint32_t uchar) {
                uint8_t buf_char[5];
                if (size_t const char_len = UTF32toUTF8(uchar, buf_char, sizeof(buf_char))) {
                    this->copy_bytes({buf_char, char_len});
                    this->kbd_chars_size[this->kbd_char_pos] = char_len;
                    ++this->kbd_char_pos;
                }
            },
            [this, uchar](array_view_const_char no_printable_str) {
                if (uchar == 0x08 && this->kbd_char_pos) {
                    --this->kbd_char_pos;
                    this->kbd_stream.rewind(
                        this->kbd_stream.get_offset()
                      - this->kbd_chars_size[this->kbd_char_pos]
                    );
                }
                else if (this->key_markers_hidden_state) {
                    if (uchar == '/') {
                        auto const single_slash = cstr_array_view("/");
                        this->copy_bytes(single_slash);
                        this->kbd_chars_size[this->kbd_char_pos] = single_slash.size();
                        ++this->kbd_char_pos;
                    }
                    else {
                        this->kbd_char_pos = 0;
                    }
                }
                else if (uchar == '/') {
                    this->copy_bytes(no_printable_str);
                    this->kbd_chars_size[this->kbd_char_pos] = no_printable_str.size();
                    ++this->kbd_char_pos;
                }
                else {
                    this->copy_bytes(no_printable_str);
                    this->kbd_char_pos = 0;
                }
            },
            filter_slash{}
        );
    }

    void copy_bytes(const_byte_array bytes) {
        if (this->kbd_stream.tailroom() < bytes.size()) {
            this->send_kbd();
        }
        this->kbd_stream.out_copy_bytes(bytes.data(), bytes.size());
    }

    void send_data(time_t rawtime, array_view_const_char data, char sep) {
        this->send_date(rawtime, sep);
        this->trans.send(data.data(), data.size());
        this->trans.send("\n", 1);
        this->last_time = rawtime;
    }

    void send_date(time_t rawtime, char sep) {
        tm ptm;
        localtime_r(&rawtime, &ptm);

        char string_date[256];

        auto const data_sz = std::sprintf(
            string_date, "%4d-%02d-%02d %02d:%02d:%02d %c ",
            ptm.tm_year+1900, ptm.tm_mon+1, ptm.tm_mday,
            ptm.tm_hour, ptm.tm_min, ptm.tm_sec, sep
        );

        this->trans.send(string_date, data_sz);
    }

    void send_kbd_if_special_char(uint32_t uchar) {
        if (uchar == '\r' || uchar == '\t') {
            if (!this->previous_char_is_event_flush) {
                this->send_kbd();
            }
        }
        else {
            this->previous_char_is_event_flush = false;
        }
    }

    void send_kbd() {
        if (this->kbd_stream.get_offset()) {
            this->formatted_message.assign("KBD_INPUT", {
                {"data", stream_to_avchar(this->kbd_stream)}
            });
            this->send_data(this->last_time, this->formatted_message.av(), '-');
            this->kbd_stream.rewind();
            this->previous_char_is_event_flush = true;
            this->kbd_char_pos = 0;
        }
    }
};

class SessionLogAgent : public gdi::CaptureProbeApi
{
    KeyQvalueFormatter line;
    SessionMeta & session_meta;

public:
    SessionLogAgent(SessionMeta & session_meta)
    : session_meta(session_meta)
    {}

    void session_update(const timeval& now, array_view_const_char message) override {
        agent_data_extractor(this->line, message);
        if (!this->line.str().empty()) {
            this->session_meta.send_line(now.tv_sec, this->line.av());
        }
    }

    void possible_active_window_change() override {
    }
};

class MetaCaptureImpl
{
public:
    OutFileTransport meta_trans;
    SessionMeta meta;
    SessionLogAgent session_log_agent;
    bool enable_agent;

    MetaCaptureImpl(
        const timeval & now,
        MetaParams const & meta_params,
        std::string record_path,
        const char * const basename,
        ReportError report_error)
    : meta_trans(unique_fd{[&](){
        record_path.append(basename).append(".meta");
        const char * filename = record_path.c_str();
        int fd = ::open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0440);
        if (fd < 0) {
            LOG(LOG_ERR, "failed opening=%s\n", filename);
            throw Error(ERR_TRANSPORT_OPEN_FAILED);
        }
        return fd;
    }()}, std::move(report_error))
    , meta(now, this->meta_trans, underlying_cast(meta_params.hide_non_printable))
    , session_log_agent(this->meta)
    , enable_agent(underlying_cast(meta_params.enable_session_log))
    {
    }

    SessionMeta & get_session_meta() {
        return this->meta;
    }
};


class TitleCaptureImpl : public gdi::CaptureApi, public gdi::CaptureProbeApi
{
    OcrTitleExtractorBuilder ocr_title_extractor_builder;
    AgentTitleExtractor agent_title_extractor;

    std::reference_wrapper<TitleExtractorApi> title_extractor;

    timeval  last_ocr;
    std::chrono::microseconds usec_ocr_interval;

    NotifyTitleChanged & notify_title_changed;

    KeyQvalueFormatter formatted_message;
    ReportMessageApi * report_message;

public:
    TitleCaptureImpl(
        const timeval & now,
        RDPDrawable & drawable,
        OcrParams ocr_params,
        NotifyTitleChanged & notify_title_changed,
        ReportMessageApi * report_message)
    : ocr_title_extractor_builder(
        drawable.impl(),
        ocr_params.verbosity,
        ocr_params.ocr_version,
        ocr_params.ocr_locale,
        ocr_params.on_title_bar_only,
        ocr_params.max_unrecog_char_rate)
    , title_extractor(this->ocr_title_extractor_builder.get_title_extractor())
    , last_ocr(now)
    , usec_ocr_interval(ocr_params.interval)
    , notify_title_changed(notify_title_changed)
    , report_message(report_message)
    {
    }


    Microseconds periodic_snapshot(
        const timeval& now, int /*cursor_x*/, int /*cursor_y*/, bool /*ignore_frame_in_timeval*/
    ) override {
        std::chrono::microseconds const diff {difftimeval(now, this->last_ocr)};

        if (diff >= this->usec_ocr_interval) {
            this->last_ocr = now;

            auto title = this->title_extractor.get().extract_title();

            if (title.data()/* && title.size()*/) {
                notify_title_changed.notify_title_changed(now, title);
                if (&this->title_extractor.get() != &this->agent_title_extractor
                 && this->report_message)
                {
                    this->formatted_message.assign("TITLE_BAR", {{"data", title}});
                    this->report_message->log5(this->formatted_message.str());
                }
            }

            return this->usec_ocr_interval;
        }
        else {
            return this->usec_ocr_interval - diff;
        }
    }

    void session_update(timeval const & /*now*/, array_view_const_char message) override {
        bool const enable_probe = (::strcasecmp(message.data(), "Probe.Status=Unknown") != 0);
        if (enable_probe) {
            this->title_extractor = this->agent_title_extractor;
            this->agent_title_extractor.session_update(message);
        }
        else {
            this->title_extractor = this->ocr_title_extractor_builder.get_title_extractor();
        }

    }

    void possible_active_window_change() override {}
};



void Capture::Graphic::draw_impl(RDP::FrameMarker const & cmd)
{
    for (gdi::GraphicApi & gd : this->gds) {
        gd.draw(cmd);
    }

    if (cmd.action == RDP::FrameMarker::FrameEnd) {
        for (gdi::CaptureApi & cap : this->caps) {
            cap.frame_marker_event(this->mouse.last_now, this->mouse.last_x, this->mouse.last_y, false);
        }
    }
}

void Capture::Graphic::draw_impl(const RDP::RAIL::NewOrExistingWindow & cmd)
{
    for (gdi::GraphicApi & gd : this->gds) {
        gd.draw(cmd);
    }

    // cmd.log(LOG_INFO);
    for (gdi::CaptureApi & cap : this->caps) {
        cap.new_or_existing_window_event(cmd.header.WindowId(),
            cmd.header.FieldsPresentFlags(),
            cmd.Style(), cmd.ShowState(),
            cmd.VisibleOffsetX(), cmd.VisibleOffsetY(),
            cmd.VisibilityRects());
    }
}

void Capture::Graphic::draw_impl(const RDP::RAIL::DeletedWindow & cmd)
{
    for (gdi::GraphicApi & gd : this->gds) {
        gd.draw(cmd);
    }

    for (gdi::CaptureApi & cap : this->caps) {
        cap.delete_window_event(cmd.header.WindowId());
    }
}


void Capture::TitleChangedFunctions::notify_title_changed(
    timeval const & now, array_view_const_char title
) {
    if (this->capture.patterns_checker) {
        this->capture.patterns_checker->operator()(title);
    }
    if (this->capture.meta_capture_obj) {
        this->capture.meta_capture_obj->get_session_meta().title_changed(now.tv_sec, title);
    }
    if (this->capture.sequenced_video_capture_obj) {
        this->capture.sequenced_video_capture_obj->next_video(now);
    }
    if (this->capture.update_progress_data) {
        this->capture.update_progress_data->next_video(now.tv_sec);
    }
}

void Capture::NotifyMetaIfNextVideo::notify_next_video(
    const timeval& now, NotifyNextVideo::reason reason
) {
    assert(this->session_meta);
    if (reason == NotifyNextVideo::reason::sequenced) {
        this->session_meta->next_video(now.tv_sec);
    }
}


Capture::Capture(
    const CaptureParams capture_params,
    const DrawableParams drawable_params,
    bool capture_wrm, const WrmParams wrm_params,
    bool capture_png, const PngParams png_params,
    bool capture_pattern_checker, const PatternParams pattern_params,
    bool capture_ocr, const OcrParams ocr_params,
    bool capture_video, const SequencedVideoParams /*sequenced_video_params*/,
    bool capture_video_full, const FullVideoParams full_video_params,
    bool capture_meta, const MetaParams meta_params,
    bool capture_kbd, const KbdLogParams kbd_log_params,
    const VideoParams video_params,
    UpdateProgressData * update_progress_data,
    Rect crop_rect)
: is_replay_mod(!capture_params.report_message)
, update_progress_data(update_progress_data)
, mouse_info{capture_params.now, drawable_params.width / 2, drawable_params.height / 2}
, capture_event{}
, capture_drawable(capture_wrm || capture_video || capture_ocr || capture_png || capture_video_full)
{
   //assert(report_message ? order_bpp == capture_bpp : true);

    if (capture_png || (capture_params.report_message && (capture_video || capture_ocr))) {
        if (recursive_create_directory(capture_params.record_tmp_path,
                S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IXGRP, -1) != 0) {
            LOG(LOG_INFO, "Failed to create directory: \"%s\"", capture_params.record_tmp_path);
        }
    }

    if (capture_wrm || capture_video || capture_ocr || capture_png || capture_video_full) {
        if (drawable_params.rdp_drawable) {
            this->gd_drawable = drawable_params.rdp_drawable;
        }
        else {
            this->gd_drawable_.reset(new RDPDrawable(
                drawable_params.width, drawable_params.height));
            this->gd_drawable = this->gd_drawable_.get();
        }
        this->gds.push_back(*this->gd_drawable);

        not_null_ptr<gdi::ImageFrameApi> image_frame_api_ptr = this->gd_drawable;

        if (!crop_rect.isempty()) {
            assert(!capture_png || !png_params.real_time_image_capture);
            this->video_cropper.reset(new VideoCropper(
                    *this->gd_drawable,
                    crop_rect.x,
                    crop_rect.y,
                    crop_rect.cx,
                    crop_rect.cy
                ));

            image_frame_api_ptr = this->video_cropper.get();
        }

        this->graphic_api.reset(new Graphic(this->mouse_info, this->gds, this->caps));

        if (capture_png) {
            if (png_params.real_time_image_capture) {
                if (png_params.remote_program_session) {
                    assert(image_frame_api_ptr == this->gd_drawable);

                    this->video_cropper.reset(new VideoCropper(
                            *this->gd_drawable,
                            0,
                            0,
                            this->gd_drawable->width(),
                            this->gd_drawable->height()
                        ));

                    image_frame_api_ptr = this->video_cropper.get();
                }

                this->png_capture_real_time_obj.reset(new PngCaptureRT(
                    capture_params, png_params, *this->gd_drawable, *image_frame_api_ptr));
            }
            else {
                this->png_capture_obj.reset(new PngCapture(
                    capture_params, png_params, *this->gd_drawable, *image_frame_api_ptr));
            }
        }

        if (capture_wrm) {
            this->wrm_capture_obj.reset(new WrmCaptureImpl(
                capture_params, wrm_params, *this->gd_drawable));
        }

        if (capture_meta) {
            this->meta_capture_obj.reset(new MetaCaptureImpl(
                capture_params.now, meta_params,
                capture_params.record_tmp_path, capture_params.basename,
                report_error_from_reporter(capture_params.report_message)
            ));
        }

        if (capture_video) {
            std::reference_wrapper<NotifyNextVideo> notifier = this->null_notifier_next_video;
            if (video_params.capture_chunk && this->meta_capture_obj) {
                this->notifier_next_video.session_meta = &this->meta_capture_obj->get_session_meta();
                notifier = this->notifier_next_video;
            }
            this->sequenced_video_capture_obj.reset(new SequencedVideoCaptureImpl(
                capture_params, png_params.zoom, *this->gd_drawable,
                *image_frame_api_ptr, video_params, notifier));
        }

        if (capture_video_full) {
            this->full_video_capture_obj.reset(new FullVideoCaptureImpl(
                capture_params, *this->gd_drawable,
                *image_frame_api_ptr, video_params, full_video_params));
        }

        if (capture_pattern_checker) {
            this->patterns_checker.reset(new PatternsChecker(
                *capture_params.report_message, pattern_params));
            if (!this->patterns_checker->contains_pattern()) {
                LOG(LOG_WARNING, "Disable pattern_checker");
                this->patterns_checker.reset();
            }
        }

        if (capture_ocr) {
            if (this->patterns_checker || this->meta_capture_obj || this->sequenced_video_capture_obj) {
                this->title_capture_obj.reset(new TitleCaptureImpl(
                    capture_params.now, *this->gd_drawable, ocr_params,
                    this->notifier_title_changed, capture_params.report_message
                ));
            }
            else {
                LOG(LOG_INFO, "Disable title_extractor");
            }
        }

        if (capture_wrm) {
            this->gds.push_back(*this->wrm_capture_obj);
            // TODO kbd_log_params.wrm_keyboard_log
            this->kbds.push_back(*this->wrm_capture_obj);
            this->caps.push_back(*this->wrm_capture_obj);
            this->ext_caps.push_back(*this->wrm_capture_obj);
            this->probes.push_back(*this->wrm_capture_obj);
        }

        if (this->png_capture_real_time_obj) {
            this->caps.push_back(*this->png_capture_real_time_obj);
        }

        if (this->png_capture_obj) {
            this->caps.push_back(*this->png_capture_obj);
        }

        if (this->sequenced_video_capture_obj) {
            //this->caps.push_back(this->sequenced_video_capture_obj->vc);
            this->caps.push_back(*this->sequenced_video_capture_obj);
        }

        if (this->full_video_capture_obj) {
            this->caps.push_back(*this->full_video_capture_obj);
        }
    }

    if (capture_kbd) {
        if (kbd_log_params.syslog_keyboard_log) {
            this->syslog_kbd_capture_obj.reset(new SyslogKbd(capture_params.now));
            this->kbds.push_back(*this->syslog_kbd_capture_obj.get());
            this->caps.push_back(*this->syslog_kbd_capture_obj.get());
        }

        if (kbd_log_params.session_log_enabled) {
            this->session_log_kbd_capture_obj.reset(new SessionLogKbd(
                *capture_params.report_message));
            this->kbds.push_back(*this->session_log_kbd_capture_obj.get());
            this->probes.push_back(*this->session_log_kbd_capture_obj.get());
        }

        this->pattern_kbd_capture_obj.reset(new PatternKbd(
            capture_params.report_message,
            pattern_params.pattern_kill,
            pattern_params.pattern_notify,
            pattern_params.verbose));

        if (this->pattern_kbd_capture_obj->contains_pattern()) {
            this->kbds.push_back(*this->pattern_kbd_capture_obj.get());
        }
        else {
            this->pattern_kbd_capture_obj.reset();
        }
    }

    if (this->meta_capture_obj) {
        this->caps.push_back(this->meta_capture_obj->meta);
        if (kbd_log_params.meta_keyboard_log) {
            this->kbds.push_back(this->meta_capture_obj->meta);
            this->probes.push_back(this->meta_capture_obj->meta);
        }
        else if (this->meta_capture_obj->enable_agent) {
            // meta includes session_log_agent
            this->probes.push_back(this->meta_capture_obj->session_log_agent);
        }
    }
    if (this->title_capture_obj) {
        this->caps.push_back(*this->title_capture_obj);
        this->probes.push_back(*this->title_capture_obj);
    }
}

Capture::~Capture()
{
    if (this->is_replay_mod) {
        this->png_capture_obj.reset();
        if (this->png_capture_real_time_obj) { this->png_capture_real_time_obj.reset(); }
        this->wrm_capture_obj.reset();
        if (this->sequenced_video_capture_obj) {
            try {
                this->sequenced_video_capture_obj.reset();
            }
            catch (Error const & e) {
                LOG(LOG_ERR, "Sequenced video: last encoding video frame error: %s", e.errmsg());
            }
        }
        if (this->full_video_capture_obj) {
            try {
                this->full_video_capture_obj.reset();
            }
            catch (Error const & e) {
                LOG(LOG_ERR, "Full video: last encoding video frame error: %s", e.errmsg());
            }
        }
    }
    else {
        this->title_capture_obj.reset();
        this->session_log_kbd_capture_obj.reset();
        this->syslog_kbd_capture_obj.reset();
        this->pattern_kbd_capture_obj.reset();
        this->sequenced_video_capture_obj.reset();
        this->png_capture_obj.reset();
        if (this->png_capture_real_time_obj) { this->png_capture_real_time_obj.reset(); }

        if (this->wrm_capture_obj) {
            timeval now = tvtime();
            this->wrm_capture_obj->send_timestamp_chunk(now, false);
            this->wrm_capture_obj.reset();
        }
    }
}

// TODO: this could be done directly in external png_capture_real_time_obj object
void Capture::update_config(bool enable_rt_display)
{
    if (this->png_capture_real_time_obj) {
        this->png_capture_real_time_obj->update_config(enable_rt_display);
    }
}

void Capture::set_row(size_t rownum, const uint8_t * data)
{
    if (this->capture_drawable) {
        this->gd_drawable->set_row(rownum, data);
    }
}

Capture::Microseconds Capture::periodic_snapshot(
    timeval const & now,
    int cursor_x, int cursor_y,
    bool ignore_frame_in_timeval
) {
    this->capture_event.reset_trigger_time();

    if (this->gd_drawable) {
        this->gd_drawable->set_mouse_cursor_pos(cursor_x, cursor_y);
    }
    this->mouse_info = {now, cursor_x, cursor_y};

    std::chrono::microseconds time = std::chrono::microseconds::max();
    if (!this->caps.empty()) {
        for (gdi::CaptureApi & cap : this->caps) {
            time = std::min(time, cap.periodic_snapshot(now, cursor_x, cursor_y, ignore_frame_in_timeval).ms());
        }
        this->capture_event.update_trigger_time(time.count());
    }
    return time;
}

void Capture::set_pointer_display() {
    if (this->capture_drawable) {
        this->gd_drawable->show_mouse_cursor(false);
    }
}
