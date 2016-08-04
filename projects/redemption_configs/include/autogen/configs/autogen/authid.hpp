//
// ATTENTION -- This file is auto-generated
//

#pragma once

enum authid_t {
    AUTHID_GLOBALS_CAPTURE_CHUNK,
    AUTHID_GLOBALS_AUTH_USER,
    AUTHID_GLOBALS_HOST,
    AUTHID_GLOBALS_TARGET,
    AUTHID_GLOBALS_TARGET_DEVICE,
    AUTHID_GLOBALS_DEVICE_ID,
    AUTHID_GLOBALS_TARGET_USER,
    AUTHID_GLOBALS_TARGET_APPLICATION,
    AUTHID_GLOBALS_TARGET_APPLICATION_ACCOUNT,
    AUTHID_GLOBALS_TARGET_APPLICATION_PASSWORD,
    AUTHID_GLOBALS_TRACE_TYPE,
    AUTHID_GLOBALS_IS_REC,
    AUTHID_GLOBALS_MOVIE_PATH,
    AUTHID_CLIENT_KEYBOARD_LAYOUT,
    AUTHID_CLIENT_DISABLE_TSK_SWITCH_SHORTCUTS,
    AUTHID_MOD_RDP_BOGUS_SC_NET_SIZE,
    AUTHID_MOD_RDP_PROXY_MANAGED_DRIVES,
    AUTHID_MOD_RDP_IGNORE_AUTH_CHANNEL,
    AUTHID_MOD_RDP_ALTERNATE_SHELL,
    AUTHID_MOD_RDP_SHELL_WORKING_DIRECTORY,
    AUTHID_MOD_RDP_USE_CLIENT_PROVIDED_ALTERNATE_SHELL,
    AUTHID_MOD_RDP_ENABLE_SESSION_PROBE,
    AUTHID_MOD_RDP_SESSION_PROBE_USE_CLIPBOARD_BASED_LAUNCHER,
    AUTHID_MOD_RDP_ENABLE_SESSION_PROBE_LAUNCH_MASK,
    AUTHID_MOD_RDP_SESSION_PROBE_ON_LAUNCH_FAILURE,
    AUTHID_MOD_RDP_SESSION_PROBE_LAUNCH_TIMEOUT,
    AUTHID_MOD_RDP_SESSION_PROBE_LAUNCH_FALLBACK_TIMEOUT,
    AUTHID_MOD_RDP_SESSION_PROBE_START_LAUNCH_TIMEOUT_TIMER_ONLY_AFTER_LOGON,
    AUTHID_MOD_RDP_SESSION_PROBE_KEEPALIVE_TIMEOUT,
    AUTHID_MOD_RDP_SESSION_PROBE_ON_KEEPALIVE_TIMEOUT_DISCONNECT_USER,
    AUTHID_MOD_RDP_SESSION_PROBE_END_DISCONNECTED_SESSION,
    AUTHID_MOD_RDP_SERVER_CERT_STORE,
    AUTHID_MOD_RDP_SERVER_CERT_CHECK,
    AUTHID_MOD_RDP_SERVER_ACCESS_ALLOWED_MESSAGE,
    AUTHID_MOD_RDP_SERVER_CERT_CREATE_MESSAGE,
    AUTHID_MOD_RDP_SERVER_CERT_SUCCESS_MESSAGE,
    AUTHID_MOD_RDP_SERVER_CERT_FAILURE_MESSAGE,
    AUTHID_MOD_RDP_SERVER_CERT_ERROR_MESSAGE,
    AUTHID_MOD_VNC_CLIPBOARD_UP,
    AUTHID_MOD_VNC_CLIPBOARD_DOWN,
    AUTHID_MOD_VNC_SERVER_CLIPBOARD_ENCODING_TYPE,
    AUTHID_MOD_VNC_BOGUS_CLIPBOARD_INFINITE_LOOP,
    AUTHID_VIDEO_RT_DISPLAY,
    AUTHID_CRYPTO_KEY0,
    AUTHID_CRYPTO_KEY1,
    AUTHID_TRANSLATION_LANGUAGE,
    AUTHID_TRANSLATION_PASSWORD_EN,
    AUTHID_TRANSLATION_PASSWORD_FR,
    AUTHID_CONTEXT_OPT_BITRATE,
    AUTHID_CONTEXT_OPT_FRAMERATE,
    AUTHID_CONTEXT_OPT_QSCALE,
    AUTHID_CONTEXT_OPT_BPP,
    AUTHID_CONTEXT_OPT_HEIGHT,
    AUTHID_CONTEXT_OPT_WIDTH,
    AUTHID_CONTEXT_AUTH_ERROR_MESSAGE,
    AUTHID_CONTEXT_SELECTOR,
    AUTHID_CONTEXT_SELECTOR_CURRENT_PAGE,
    AUTHID_CONTEXT_SELECTOR_DEVICE_FILTER,
    AUTHID_CONTEXT_SELECTOR_GROUP_FILTER,
    AUTHID_CONTEXT_SELECTOR_PROTO_FILTER,
    AUTHID_CONTEXT_SELECTOR_LINES_PER_PAGE,
    AUTHID_CONTEXT_SELECTOR_NUMBER_OF_PAGES,
    AUTHID_CONTEXT_TARGET_PASSWORD,
    AUTHID_CONTEXT_TARGET_HOST,
    AUTHID_CONTEXT_TARGET_SERVICE,
    AUTHID_CONTEXT_TARGET_PORT,
    AUTHID_CONTEXT_TARGET_PROTOCOL,
    AUTHID_CONTEXT_PASSWORD,
    AUTHID_CONTEXT_REPORTING,
    AUTHID_CONTEXT_AUTH_CHANNEL_ANSWER,
    AUTHID_CONTEXT_AUTH_CHANNEL_TARGET,
    AUTHID_CONTEXT_MESSAGE,
    AUTHID_CONTEXT_ACCEPT_MESSAGE,
    AUTHID_CONTEXT_DISPLAY_MESSAGE,
    AUTHID_CONTEXT_REJECTED,
    AUTHID_CONTEXT_AUTHENTICATED,
    AUTHID_CONTEXT_KEEPALIVE,
    AUTHID_CONTEXT_SESSION_ID,
    AUTHID_CONTEXT_END_DATE_CNX,
    AUTHID_CONTEXT_END_TIME,
    AUTHID_CONTEXT_MODE_CONSOLE,
    AUTHID_CONTEXT_TIMEZONE,
    AUTHID_CONTEXT_REAL_TARGET_DEVICE,
    AUTHID_CONTEXT_AUTHENTICATION_CHALLENGE,
    AUTHID_CONTEXT_TICKET,
    AUTHID_CONTEXT_COMMENT,
    AUTHID_CONTEXT_DURATION,
    AUTHID_CONTEXT_WAITINFORETURN,
    AUTHID_CONTEXT_SHOWFORM,
    AUTHID_CONTEXT_FORMFLAG,
    AUTHID_CONTEXT_MODULE,
    AUTHID_CONTEXT_FORCEMODULE,
    AUTHID_CONTEXT_PROXY_OPT,
    AUTHID_CONTEXT_PATTERN_KILL,
    AUTHID_CONTEXT_PATTERN_NOTIFY,
    AUTHID_CONTEXT_OPT_MESSAGE,
    AUTHID_CONTEXT_OUTBOUND_CONNECTION_MONITORING_RULES,
    AUTHID_CONTEXT_PROCESS_MONITORING_RULES,
    AUTHID_CONTEXT_DISCONNECT_REASON,
    AUTHID_CONTEXT_DISCONNECT_REASON_ACK,
    MAX_AUTHID,
    AUTHID_UNKNOWN
};
constexpr char const * const authstr[] = {
    "capture_chunk",
    "login",
    "ip_client",
    "ip_target",
    "target_device",
    "device_id",
    "target_login",
    "target_application",
    "target_application_account",
    "target_application_password",
    "trace_type",
    "is_rec",
    "rec_path",
    "keyboard_layout",
    "disable_tsk_switch_shortcuts",
    "rdp_bogus_sc_net_size",
    "proxy_managed_drives",
    "ignore_auth_channel",
    "alternate_shell",
    "shell_working_directory",
    "use_client_provided_alternate_shell",
    "session_probe",
    "session_probe_use_smart_launcher",
    "enable_session_probe_launch_mask",
    "session_probe_on_launch_failure",
    "session_probe_launch_timeout",
    "session_probe_launch_fallback_timeout",
    "session_probe_start_launch_timeout_timer_only_after_logon",
    "session_probe_keepalive_timeout",
    "session_probe_on_keepalive_timeout_disconnect_user",
    "session_probe_end_disconnected_session",
    "server_cert_store",
    "server_cert_check",
    "server_access_allowed_message",
    "server_cert_create_message",
    "server_cert_success_message",
    "server_cert_failure_message",
    "server_cert_error_message",
    "clipboard_up",
    "clipboard_down",
    "vnc_server_clipboard_encoding_type",
    "vnc_bogus_clipboard_infinite_loop",
    "rt_display",
    "encryption_key",
    "sign_key",
    "language",
    "password_en",
    "password_fr",
    "bitrate",
    "framerate",
    "qscale",
    "bpp",
    "height",
    "width",
    "auth_error_message",
    "selector",
    "selector_current_page",
    "selector_device_filter",
    "selector_group_filter",
    "selector_proto_filter",
    "selector_lines_per_page",
    "selector_number_of_pages",
    "target_password",
    "target_host",
    "target_service",
    "target_port",
    "proto_dest",
    "password",
    "reporting",
    "auth_channel_answer",
    "auth_channel_target",
    "message",
    "accept_message",
    "display_message",
    "rejected",
    "authenticated",
    "keepalive",
    "session_id",
    "timeclose",
    "end_time",
    "mode_console",
    "timezone",
    "real_target_device",
    "authentication_challenge",
    "ticket",
    "comment",
    "duration",
    "waitinforeturn",
    "showform",
    "formflag",
    "module",
    "forcemodule",
    "proxy_opt",
    "pattern_kill",
    "pattern_notify",
    "opt_message",
    "outbound_connection_blocking_rules",
    "session_probe_process_monitoring_rules",
    "disconnect_reason",
    "disconnect_reason_ack",
};
