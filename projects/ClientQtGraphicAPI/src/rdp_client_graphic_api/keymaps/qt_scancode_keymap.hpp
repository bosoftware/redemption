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
   Copyright (C) Wallix 2010-2013
   Author(s): Clément Moroldo
*/


/*********************************************************************************************************************************************************************************************************************
 *
 *  =================
 *    DOCUMENTATION
 *  =================
 *
 *  Qt_ScanCode_KeyMap matches Qt key codes you get when catching a QKeyEvent to standard scan codes.
 *
 *
 *
 *  Basic example:
 *  --------------
 *
 *      #include "Qt_ScanCode_KeyMap.hpp"
 *
 *
 *
 *
 *      void keyPressEvent(QKeyEvent * keyEvent) {                              // standard Qt function to catch a key press event
 *
 *           Qt_ScanCode_KeyMap qsckm();                                        // construct a Qt_ScanCode_KeyMap.
 *           Qt_ScanCode_KeyMap qsckm(KEYBOARDS::EN_US);                        // Keyboard language can be set using KEYBOARDS enum.
 *           Qt_ScanCode_KeyMap qsckm(KEYBOARDS::EN_US, verbose);               // verbose is an integer.
 *
 *           qsckm.setKeyboardLayout(KEYBOARDS::EN_US);                         // set keyboard type from KEYBOARDS::
 *
 *           qsckm.keyQtEvent(Qt_ScanCode_KeyMap::KBD_FLAGS_PRESS, keyEvent);   // call keyEvent(uint16_t keyStatusFlag, QKeyEvent * keyEvent),
 *                                                                              // if key is pressed then keyStatusFlag = Qt_ScanCode_KeyMap::KBD_FLAGS_PRESS or 0,
 *                                                                              // if key is released keyStatusFlag = Qt_ScanCode_KeyMap::KBD_FLAGS_RELEASE or 0x8000 .
 *
 *           int Scan_Code     = qsckm.scanCode;                                // retrieve the scan code (The Unicode character input code. ref: msdn.microsoft.com 2.2.8.1.1.3.1.1.2 )
 *
 *           uint16_t Keyboard_Flag = qsckm.flag;                               // retrieve the 2 bytes keyboard flag (A 16-bit unsigned integer. The flags describing the Unicode keyboard event.
 *                                                                              //                                     ref: msdn.microsoft.com 2.2.8.1.1.3.1.1.2 )
 *       }
 *
 *
 *
 *  Customize:
 *  ----------
 *
 *      Qt_ScanCode_KeyMap qsckm();
 *
 *      qsckm.setCustomKeyCode(qt_key, scan_Code, ASCII8_Code, extended);       // Call setCustomKeyCode(int qt_key, int ASCII8_Code, int scan_Code, bool extended) to add new match between a Qt key
 *                                                                              // code and an ASCII8 code Qt_ScanCode_KeyMap will find within the active keyboard layout. Else you can directly
 *                                                                              // match a Qt key code with scan code as well. Set a variable to 0 to avoid the match.
 *
 *      qsckm.clearCustomKeyCode();                                              // Call clearCustomKeyCode() to empty custom entries.
 *
 *
 *
 *  Get data:
 *  ---------
 *
 *      const Keylayout_r * keylayoutsList[];                                   // The keyboard language list is a static const variable.
 *
 *      char * name = keylayoutsList[i]->locale_name;                           // keylayout IDs and names are public.
 *
 *      int ID = keylayoutsList[i]->LCID;
 *
 *
 *
 *  Error Messages:
 *  ---------------
 *
 *      # Error: Key(0x1001257) not recognized.                                                         // Qt didn't identify this key, its Qt key code can be add as custom match or improve the code.
 *
 *      # Unknown key(0x1001257) to current layout 0x40c fr-FR.                                         // Qt identified this key as a character, but it didn't match inside the specific current character layout
 *                                                                                                      // change the current layout or improve current layout table
 *
 *      # Unknown keyboard layout (0x1). Reverting to default (English - United States - International) // If you call the constructor setting an unknow layout ID, default layout is set instead
 *
 *
 ***********************************************************************************************************************************************************************************************************************/


#pragma once


#include <iostream>
#include "keyboard/reversed_keymaps/keylayouts_r.hpp"
#include <QtGui/QKeyEvent>







class Qt_ScanCode_KeyMap
{


public:
    int                                 _verbose;
private:
    uint8_t                             _keyboardMods;
    const Keylayout_r                 * _keylayout_WORK;
    const Keylayout_r::KeyLayoutMap_t * _layoutMods[9];
    const Keylayout_r::KeyLayoutMap_t * _layout;

    Keylayout_r::KeyLayoutMap_t         _customNoExtended;
    Keylayout_r::KeyLayoutMap_t         _customExtended;
    Keylayout_r::KeyLayoutMap_t         _customNoExtendedKeylayoutApplied;
    Keylayout_r::KeyLayoutMap_t         _customExtendedKeylayoutApplied;

    bool                                _deadKeys;
    bool                                _unvalidScanCode;



    enum : int {
          ALT_MOD      = 0x10
        , CTRL_MOD     = 0x08
        , CAPSLOCK_MOD = 0x04
        , ALTGR_MOD    = 0x02
        , SHIFT_MOD    = 0x01
        , NO_MOD       = 0x00
    };

    enum : int {
          MAX_CHAR_QT_KEY_VALUE     = 256
        , MIN_NON_CHAR_QT_KEY_VALUE = 0x00FFFFFF
        , MIN_DEAD_QT_KEY_VALUE     = 0x01001249
        , MAX_DEAD_QT_KEY_VALUE     = 0x01001263
        , MASK_ALT                  = 0x0F
    };

    enum : int {
           KBD_FLAGS_PRESS    = 0x0000
         , KBD_FLAGS_EXTENDED = 0x0100
         , KBD_FLAGS_DOWN     = 0x4000
         , KBD_FLAGS_RELEASE  = 0x8000
    };



    void applyCharTable() {
        try {
            if (this->_deadKeys) {
                const Keylayout_r::KeyLayoutMap_t & layout = this->_keylayout_WORK->deadkeys;
                this->scanCode = layout.at(scanCode);
                this->_deadKeys = false;
            } else {
                const Keylayout_r::KeyLayoutMap_t & layout = *(this->_layout);
                this->scanCode = layout.at(scanCode);
            }
        } catch (const std::exception &) {
            std::cerr << std::hex << "Unknown key(0x" << this->scanCode << ") to current layout 0x" << this->_keylayout_WORK->LCID << " " << this->_keylayout_WORK->locale_name << "." << std::endl;
            this->_unvalidScanCode = true;
        }
    }


    void showkey(const int keyStatusFlag, const QKeyEvent *e) {
        int abc(e->text().toStdString()[0]);
        std::string keyStatus;
        if (keyStatusFlag == 0) {
            std::cout << std::hex << "keyPressed=0x" << e->key() << " scanCode=0x" << this->scanCode << " text=\'" << e->text().toStdString() << "\' text(hexa)=0x" << abc << "." << std::endl;
        } else {
            std::cout << std::hex << "keyRelease=0x" << e->key() << " scanCode=0x" << this->scanCode << " text=\'" << e->text().toStdString() << "\' text(hexa)=0x" << abc << "." << std::endl;
        }
    }


    void layout_Work_Update() {
        //                                         ___________________________________________________________________
        //                                        |       |       |        |       |       |        |        |        |
        //   bitcode for _keyboardMods (unint8):  |   0   |   0   |   0    | ALT   | CTRL  | CAPS   | ALTGR  | SHFT   |
        //                                        |_______|_______|________|_______|_______|________|________|________|
        //
        if ((this->_keyboardMods & CTRL_MOD) == CTRL_MOD) {
            this->_layout = this->_layoutMods[8];  // CTRL
        } else {
            this->_layout = this->_layoutMods[this->_keyboardMods & MASK_ALT];   // set appropriate mod (ALT has no mod)
        }
    }


    //===================
    //    Characters
    //===================
    void getKeyChar(const QKeyEvent *e) {

        switch (scanCode) {
            case Qt::Key_Eacute      : this->scanCode = 0xE9; break; /*é*/
            case Qt::Key_Ccedilla    : this->scanCode = 0xE7; break; /*ç*/
            case Qt::Key_Agrave      : this->scanCode = 0xE0; break; /*à*/
            case Qt::Key_Ugrave      : this->scanCode = 0xF9; break; /*ù*/
            case Qt::Key_Egrave      : this->scanCode = 0xE8; break; /*è*/
            case Qt::Key_section     : this->scanCode = 0xA7; break; /*§*/
            case Qt::Key_twosuperior : this->scanCode = 0xB2; break; // square
            case Qt::Key_sterling    : this->scanCode = 0xA3; break; // £
            case Qt::Key_currency    : this->scanCode = 0x1B; break; //
            case Qt::Key_degree      : this->scanCode = 0xB0; break; // °
            case Qt::Key_mu          : this->scanCode = 0xB5; break; /*µ*/
            case Qt::Key_Slash    :
                if (this->_keyboardMods == 0) {
                    scanCode = 0x21;
                    this->flag = this->flag | KBD_FLAGS_EXTENDED;
                }
                break;

            default: this->scanCode = e->text().toStdString()[0]; break;
        }

        //-----------------------------
        //    Keyboard Layout apply
        //-----------------------------
        this->applyCharTable();
    }//=============================================================================


    //=====================
    //      ShortCuts
    //=====================
    void getCharShortCut() {

        switch (scanCode) {
            case Qt::Key_A          : this->scanCode = 'a';  break; /*A*/         case Qt::Key_1         : this->scanCode = '1';  break; /*1*/
            case Qt::Key_Z          : this->scanCode = 'z';  break; /*Z*/         case Qt::Key_2         : this->scanCode = '2';  break; /*2*/
            case Qt::Key_Y          : this->scanCode = 'y';  break; /*Y*/         case Qt::Key_3         : this->scanCode = '3';  break; /*3*/
            case Qt::Key_X          : this->scanCode = 'x';  break; /*X*/         case Qt::Key_4         : this->scanCode = '4';  break; /*4*/
            case Qt::Key_C          : this->scanCode = 'c';  break; /*C*/         case Qt::Key_5         : this->scanCode = '5';  break; /*5*/
            case Qt::Key_V          : this->scanCode = 'v';  break; /*V*/         case Qt::Key_6         : this->scanCode = '6';  break; /*6*/
            case Qt::Key_E          : this->scanCode = 'e';  break; /*E*/         case Qt::Key_7         : this->scanCode = '7';  break; /*7*/
            case Qt::Key_R          : this->scanCode = 'r';  break; /*R*/         case Qt::Key_8         : this->scanCode = '8';  break; /*8*/
            case Qt::Key_T          : this->scanCode = 't';  break; /*T*/         case Qt::Key_9         : this->scanCode = '9';  break; /*9*/
            case Qt::Key_U          : this->scanCode = 'u';  break; /*U*/         case Qt::Key_Ampersand : this->scanCode = '&';  break; /*&*/
            case Qt::Key_I          : this->scanCode = 'i';  break; /*I*/         case Qt::Key_Asterisk  : this->scanCode = '*';  break; /***/
            case Qt::Key_B          : this->scanCode = 'b';  break; /*B*/         case Qt::Key_Underscore: this->scanCode = '_';  break; /*_*/
            case Qt::Key_D          : this->scanCode = 'd';  break; /*D*/         case Qt::Key_QuoteDbl  : this->scanCode = '"';  break; /*"*/
            case Qt::Key_F          : this->scanCode = 'f';  break; /*F*/         case Qt::Key_Apostrophe: this->scanCode = 39 ;  break; /*'*/
            case Qt::Key_G          : this->scanCode = 'g';  break; /*G*/         case Qt::Key_Comma     : this->scanCode = ',';  break; /*,*/
            case Qt::Key_H          : this->scanCode = 'h';  break; /*H*/         case Qt::Key_Plus      : this->scanCode = '+';  break; /*+*/
            case Qt::Key_J          : this->scanCode = 'j';  break; /*J*/         case Qt::Key_Minus     : this->scanCode = '-';  break; /*-*/
            case Qt::Key_K          : this->scanCode = 'k';  break; /*K*/         case Qt::Key_ParenLeft : this->scanCode = '(';  break; /*(*/
            case Qt::Key_L          : this->scanCode = 'l';  break; /*L*/         case Qt::Key_ParenRight: this->scanCode = ')';  break; /*)*/
            case Qt::Key_M          : this->scanCode = 'm';  break; /*M*/         case Qt::Key_Equal     : this->scanCode = '=';  break; /*=*/
            case Qt::Key_N          : this->scanCode = 'n';  break; /*N*/         case Qt::Key_Exclam    : this->scanCode = '!';  break; /*!*/
            case Qt::Key_O          : this->scanCode = 'o';  break; /*O*/         case Qt::Key_Colon     : this->scanCode = ':';  break; /*:*/
            case Qt::Key_P          : this->scanCode = 'p';  break; /*P*/         case Qt::Key_Dollar    : this->scanCode = '$';  break; /*$*/
            case Qt::Key_Q          : this->scanCode = 'q';  break; /*Q*/         case Qt::Key_multiply  : this->scanCode = '*';  break; /***/
            case Qt::Key_S          : this->scanCode = 's';  break; /*S*/         case Qt::Key_Space     : this->scanCode = ' ';  break; /* */
            case Qt::Key_W          : this->scanCode = 'w';  break; /*W*/         case Qt::Key_section   : this->scanCode = 0xA7; break; /*§*/
            case Qt::Key_Eacute     : this->scanCode = 0xE;  break; /*é*/         case Qt::Key_degree    : this->scanCode = 0xB0; break; /*°*/
            case Qt::Key_Ccedilla   : this->scanCode = 0xE7; break; /*ç*/
            case Qt::Key_Agrave     : this->scanCode = 0xE0; break; /*à*/         //case Qt::Key_         : this->scanCode = '';  break; /**/
            case Qt::Key_Ugrave     : this->scanCode = 0xF9; break; /*ù*/
            case Qt::Key_Egrave     : this->scanCode = 0xE8; break; /*è*/
            case Qt::Key_twosuperior: this->scanCode = 0xB2; break; // œ or square
            case Qt::Key_Slash     :
                if (this->_keyboardMods == 0) {
                    scanCode = 0x21;
                    this->flag = this->flag | KBD_FLAGS_EXTENDED;
                }
                break; /* / */

            default: break;
        }

        if ((this->_keyboardMods & CTRL_MOD) == CTRL_MOD) {
            this->_layout = _layoutMods[NO_MOD]; // noMod
        }

        //-----------------------------
        //    Keyboard Layout apply
        //----------------------------
        this->applyCharTable();

        if ((this->_keyboardMods & CTRL_MOD) == CTRL_MOD) {
            this->_layout = _layoutMods[CTRL_MOD];  // ctrl mod
        }
    }//======================================================================================================================================


    //===================
    //   NON CHAR KEYS
    //===================
    //=======================================================================================================================
    void getNonCharKeys() {
        switch (scanCode) {

        //------------------------------------------
        //    Not mod neither char keys Extended
        //------------------------------------------
            case Qt::Key_Enter      : this->scanCode = 0x1C; this->flag = this->flag | KBD_FLAGS_EXTENDED; break; //  ENTER KP
            case Qt::Key_NumLock    : this->scanCode = 0x45; this->flag = this->flag | KBD_FLAGS_EXTENDED; break; //  NUMLOCK
            case Qt::Key_Insert     : this->scanCode = 0x52; this->flag = this->flag | KBD_FLAGS_EXTENDED; break; //  INSERT
            case Qt::Key_Delete     : this->scanCode = 0x53; this->flag = this->flag | KBD_FLAGS_EXTENDED; break; //  DELETE
            case Qt::Key_End        : this->scanCode = 0x4F; this->flag = this->flag | KBD_FLAGS_EXTENDED; break; //  END
            case Qt::Key_PageDown   : this->scanCode = 0x51; this->flag = this->flag | KBD_FLAGS_EXTENDED; break; //  PG DN
            case Qt::Key_PageUp     : this->scanCode = 0x49; this->flag = this->flag | KBD_FLAGS_EXTENDED; break; //  PG UP
            case Qt::Key_Up         : this->scanCode = 0x48; this->flag = this->flag | KBD_FLAGS_EXTENDED; break; //  U ARROW
            case Qt::Key_Left       : this->scanCode = 0x4B; this->flag = this->flag | KBD_FLAGS_EXTENDED; break; //  L ARROW
            case Qt::Key_Down       : this->scanCode = 0x50; this->flag = this->flag | KBD_FLAGS_EXTENDED; break; //  D ARROW
            case Qt::Key_Right      : this->scanCode = 0x4D; this->flag = this->flag | KBD_FLAGS_EXTENDED; break; //  R ARROW
            case Qt::Key_Meta       : this->scanCode = 0x5c; this->flag = this->flag | KBD_FLAGS_EXTENDED; break; //  R WINDOW
            case Qt::Key_Menu       : this->scanCode = 0x5D; this->flag = this->flag | KBD_FLAGS_EXTENDED; break; //  MENU APPS
    //------------------------------------------------------------------------------------------------------------------------
        //---------------------------------------------
        //    Not mod neither char keys NOT Extended
        //---------------------------------------------
            case Qt::Key_Return               : this->scanCode = 0x1C; break; //  ENTER
            case Qt::Key_Backspace            : this->scanCode = 0x0E; break; //  BKSP
            case Qt::Key_Escape               : this->scanCode = 0x01; break; //  ESCAPE
            case Qt::Key_F1                   : this->scanCode = 0x3B; break; //  F1
            case Qt::Key_F2                   : this->scanCode = 0x3C; break; //  F2
            case Qt::Key_F3                   : this->scanCode = 0x3D; break; //  F3
            case Qt::Key_F4                   : this->scanCode = 0x3E; break; //  F4
            case Qt::Key_F5                   : this->scanCode = 0x3F; break; //  F5
            case Qt::Key_F6                   : this->scanCode = 0x40; break; //  F6
            case Qt::Key_F7                   : this->scanCode = 0x41; break; //  F7
            case Qt::Key_F8                   : this->scanCode = 0x42; break; //  F8
            case Qt::Key_F9                   : this->scanCode = 0x43; break; //  F9
            case Qt::Key_F10                  : this->scanCode = 0x44; break; //  F10
            case Qt::Key_F11                  : this->scanCode = 0x57; break; //  F11
            case Qt::Key_F12                  : this->scanCode = 0x58; break; //  F12
            case Qt::Key_ScrollLock           : this->scanCode = 0x46; break; //  SCROLL
            case Qt::Key_Pause                : this->scanCode = 0xE1; break; //  PAUSE
            case Qt::Key_Tab                  : this->scanCode = 0x0F; break; //  TAB
            case Qt::Key_Home                 : this->scanCode = 0x47; break; //  HOME
            case Qt::Key_MediaStop            : this->scanCode = 0x24; break; //
            case Qt::Key_MediaPrevious        : this->scanCode = 0x10; break; //
            case Qt::Key_MediaNext            : this->scanCode = 0x19; break; //
            case Qt::Key_MediaTogglePlayPause : this->scanCode = 0x22; break; //
            case Qt::Key_LaunchMail           : this->scanCode = 0x47; break; //
            case Qt::Key_Search               : this->scanCode = 0x47; break; //
            case Qt::Key_Favorites            : this->scanCode = 0x66; break; //
            case Qt::Key_HomePage             : this->scanCode = 0x32; break; //
            case Qt::Key_VolumeDown           : this->scanCode = 0x2E; break; //
            case Qt::Key_VolumeUp             : this->scanCode = 0x30; break; //
            case Qt::Key_VolumeMute           : this->scanCode = 0x20; break; //
            case Qt::Key_PowerOff             : this->scanCode = 0x5E; break; //
            case Qt::Key_WakeUp               : this->scanCode = 0x63; break; //
            case Qt::Key_Sleep                : this->scanCode = 0x5F; break; //
    //----------------------------------------------------------------------
            //--------------------------
            //    keyboard modifiers
            //--------------------------
            case Qt::Key_Alt : this->scanCode = 0x38;         //  L ALT
                if (this->flag == 0) {
                    if (this->_keyboardMods & CTRL_MOD) {
                        this->_keyboardMods += ALTGR_MOD;
                        this->_keyboardMods -= CTRL_MOD;
                    } else {
                        this->_keyboardMods += ALT_MOD;
                    }
                } else {
                    if (this->_keyboardMods & ALTGR_MOD) {
                        this->_keyboardMods -= ALTGR_MOD;
                        this->_keyboardMods += CTRL_MOD;
                    } else {
                        this->_keyboardMods -= ALT_MOD;
                    }
                }
                this->flag = this->flag | KBD_FLAGS_EXTENDED;
                this->layout_Work_Update();
                break;

            case Qt::Key_AltGr : this->scanCode = 0x38;       //  R ALT GR
                if (this->flag == 0) {
                    if (this->_keyboardMods & CTRL_MOD) {
                        this->_keyboardMods -= CTRL_MOD;
                    }
                    if (this->_keyboardMods & ALT_MOD) {
                        this->_keyboardMods -= ALT_MOD;
                    }
                    this->_keyboardMods += ALTGR_MOD;
                } else {
                    this->_keyboardMods -= ALTGR_MOD;
                }
                this->flag = this->flag | KBD_FLAGS_EXTENDED;
                this->layout_Work_Update();
                break;

            case Qt::Key_Control : this->scanCode = 0x1D;     //  R L CTRL
                if (this->flag == 0) {
                    if (this->_keyboardMods & ALT_MOD) {
                        this->_keyboardMods += ALTGR_MOD;
                        this->_keyboardMods -= ALT_MOD;
                    } else {
                        this->_keyboardMods += CTRL_MOD;
                    }
                } else {
                    if (this->_keyboardMods & ALTGR_MOD) {
                        this->_keyboardMods -= ALTGR_MOD;
                        this->_keyboardMods += ALT_MOD;
                    } else {
                        this->_keyboardMods -= CTRL_MOD;
                    }
                }
                this->flag = this->flag | KBD_FLAGS_EXTENDED;
                this->layout_Work_Update();
                break;

            case Qt::Key_Shift : this->scanCode = 0x36;       // R L SHFT
                if (this->flag == 0) {
                    this->_keyboardMods += SHIFT_MOD;
                } else {
                    this->_keyboardMods -= SHIFT_MOD;
                }
                this->layout_Work_Update();
                break;

            case Qt::Key_CapsLock : this->scanCode = 0x3A;    //  CAPSLOCK
                if (this->flag == 0) {
                    if ((this->_keyboardMods & CAPSLOCK_MOD) == CAPSLOCK_MOD) {
                        this->_keyboardMods -= CAPSLOCK_MOD;
                    } else {
                        this->_keyboardMods += CAPSLOCK_MOD;
                    }
                    this->layout_Work_Update();
                }
                break;

            default: break;
        }
    }//====================================================================================================


    //===================
    //     DEAD KEYS
    //===================
    void getDeadKeys() {

        switch (scanCode) {
            case Qt::Key_Dead_Circumflex       : this->scanCode = 0x5E; this->_deadKeys = true; break; //  ^
            case Qt::Key_Dead_Grave            : this->scanCode = 0x60; this->_deadKeys = true; break; //  ` grave
            case Qt::Key_Dead_Tilde            : this->scanCode = 0x7E; this->_deadKeys = true; break; //
            case Qt::Key_Dead_Diaeresis        : this->scanCode = 0xA8; this->_deadKeys = true; break; // ¨
            case Qt::Key_Dead_Acute            : this->scanCode = 0xB4; this->_deadKeys = true; break; //
            case Qt::Key_Dead_Macron           : this->scanCode = 0xAF; this->_deadKeys = true; break; //
            case Qt::Key_Dead_Cedilla          : this->scanCode = 0xB8; this->_deadKeys = true; break; //
            //case Qt::Key_Dead_Breve            : this->scanCode = ''; this->_deadKeys = true; break; //
            //case Qt::Key_Dead_Abovedot         : this->scanCode = ''; this->_deadKeys = true; break; //
            //case Qt::Key_Dead_Abovering        : this->scanCode = ''; this->_deadKeys = true; break; //
            //case Qt::Key_Dead_Doubleacute      : this->scanCode = ''; this->_deadKeys = true; break; //
            //case Qt::Key_Dead_Caron            : this->scanCode = ''; this->_deadKeys = true; break;
            //case Qt::Key_Dead_Ogonek           : this->scanCode = ''; this->_deadKeys = true; break;
            //case Qt::Key_Dead_Iota             : this->scanCode = ''; this->_deadKeys = true; break;
            //Case Qt::Key_Dead_Voiced_Sound     : this->scanCode = ''; this->_deadKeys = true; break;
            //case Qt::Key_Dead_Semivoiced_Sound : this->scanCode = ''; this->_deadKeys = true; break;
            //case Qt::Key_Dead_Belowdot         : this->scanCode = ''; this->_deadKeys = true; break;
            //case Qt::Key_Dead_Hook             : this->scanCode = ''; this->_deadKeys = true; break;
            //case Qt::Key_Dead_Horn             : this->scanCode = ''; this->_deadKeys = true; break;

            default: break;
        }

        //-----------------------------
        //    Keyboard Layout apply
        //----------------------------
        this->applyCharTable();
    }//===================================================================================================


    //===========================
    //  Custom Key NOT EXTENDED
    //===========================
    bool getCustomKeysNoExtendedKeylayoutApplied() {

        if (this->scanCode == 0) {
            return false;
        }
        try {
            this->scanCode = this->_customNoExtendedKeylayoutApplied.at(this->scanCode);

            //-----------------------------
            //    Keyboard Layout apply
            //-----------------------------
            this->applyCharTable();
            return true;
        } catch (const std::exception &) {
            return false;
        }
    }//------------------------------------------------------------------------

    bool getCustomKeysNoExtended() {
        if (this->scanCode == 0) {
            return false;
        }
        try {
            this->scanCode = this->_customNoExtended.at(this->scanCode);
            return true;
        } catch (const std::exception &) {
            return false;
        }
    }//========================================================================


    //==========================
    //   Custom Key EXTENDED
    //==========================
     bool getCustomKeysExtendedKeylayoutApplied() {
        if (this->scanCode == 0) {
            return false;
        }
        try {
            this->scanCode = this->_customExtendedKeylayoutApplied.at(this->scanCode);

            //-----------------------------
            //    Keyboard Layout apply
            //-----------------------------
            this->applyCharTable();
            this->flag = this->flag | KBD_FLAGS_EXTENDED;
            return true;
        } catch (const std::exception &) {
            this->scanCode = 0;
            return false;
        }
    }//------------------------------------------------------------------------------------------------------------

    bool getCustomKeysExtended() {
        if (this->scanCode == 0) {
            return false;
        }
        try {
            this->scanCode = this->_customExtended.at(this->scanCode);
            this->flag = this->flag | KBD_FLAGS_EXTENDED;
            return true;
        } catch (const std::exception &) {
            return false;
        }
    }//============================================================================================================


public:
    int scanCode;
    uint16_t flag;

    //=================//
    //   CONSTRUCTOR   //
    //===================================================================================
    Qt_ScanCode_KeyMap(int LCID = KEYBOARDS::EN_US_INTERNATIONAL, int verbose = 0):
      _verbose(verbose)
    , _keyboardMods(0)
    , _keylayout_WORK(nullptr)
    , _layoutMods()
    , _layout(nullptr)
    , _customNoExtended()
    , _customExtended()
    , _customNoExtendedKeylayoutApplied()
    , _customExtendedKeylayoutApplied()
    , _deadKeys(false)
    , _unvalidScanCode(false)
    {
        this->setKeyboardLayout(LCID);
    }//==================================================================================


    ~Qt_ScanCode_KeyMap() {}


    void setKeyboardLayout(int LCID) {
        bool found = false;
        for (uint8_t i = 0 ; i < KEYLAYOUTS_LIST_SIZE; i++) {
            if (keylayoutsList[i]->LCID == LCID){
                this->_keylayout_WORK = keylayoutsList[i];
                found = true;
                break;
            }
        }
        if (!found){
            std::cout << std::hex << "Unknown keyboard layout (0x" << LCID << "). Reverting to default (English - United States - International)." << std::endl;
            this->setKeyboardLayout(KEYBOARDS::EN_US_INTERNATIONAL);
        }

        this->_layoutMods[NO_MOD                              ] = this->_keylayout_WORK->getnoMod();
        this->_layoutMods[SHIFT_MOD                           ] = this->_keylayout_WORK->getshift();
        this->_layoutMods[ALTGR_MOD                           ] = this->_keylayout_WORK->getaltGr();
        this->_layoutMods[ALTGR_MOD    + SHIFT_MOD            ] = this->_keylayout_WORK->getshiftAltGr();
        this->_layoutMods[CAPSLOCK_MOD + NO_MOD               ] = this->_keylayout_WORK->getcapslock_noMod();
        this->_layoutMods[CAPSLOCK_MOD + SHIFT_MOD            ] = this->_keylayout_WORK->getcapslock_shift();
        this->_layoutMods[CAPSLOCK_MOD + ALTGR_MOD            ] = this->_keylayout_WORK->getcapslock_altGr();
        this->_layoutMods[CAPSLOCK_MOD + ALTGR_MOD + SHIFT_MOD] = this->_keylayout_WORK->getcapslock_shiftAltGr();
        this->_layoutMods[CTRL_MOD                            ] = this->_keylayout_WORK->getctrl();

        this->_layout = this->_layoutMods[0]; // noMod
    }


    void setCustomKeyCode(int qt_key, int scan_Code, int ASCII8_Code, bool extended) {
        if (qt_key != 0) {
            if (ASCII8_Code != 0) {
                if (extended) {
                    this->_customExtendedKeylayoutApplied.erase(qt_key);
                    this->_customExtendedKeylayoutApplied.emplace(qt_key, ASCII8_Code);
                } else {
                    this->_customNoExtendedKeylayoutApplied.erase(qt_key);
                    this->_customNoExtendedKeylayoutApplied.emplace(qt_key, ASCII8_Code);
                }
            }

            if (scan_Code != 0) {
                if (extended) {
                    this->_customExtended.erase(qt_key);
                    this->_customExtended.emplace(qt_key, scan_Code);
                } else {
                    this->_customNoExtended.erase(qt_key);
                    this->_customNoExtended.emplace(qt_key, scan_Code);
                }
            }
        }
    }


    void clearCustomKeyCode() {
        this->_customNoExtendedKeylayoutApplied.clear();
        this->_customExtendedKeylayoutApplied.clear();
        this->_customNoExtended.clear();
        this->_customExtended.clear();
    }


    void keyEvent(const uint16_t keyStatusFlag, const QKeyEvent *keyEvent) {
        this->flag = keyStatusFlag;
        this->scanCode = keyEvent->key();
        int keyCode(keyEvent->key());
        this->_deadKeys = false;
        this->_unvalidScanCode = false;

        if (keyCode != 0) {

            if (keyCode < MAX_CHAR_QT_KEY_VALUE ) {
                if ((this->_keyboardMods & CTRL_MOD) ==  CTRL_MOD || this->_keyboardMods == ALT_MOD) {

                    //--------------------------------------
                    //      Char ShortCuts CTRL or ALT
                    //--------------------------------------
                    this->getCharShortCut();
                } else {

                    //--------------------------------------
                    //             CHARACTERS
                    //--------------------------------------
                    this->getKeyChar(keyEvent);
                }
            } else {

                if (keyCode > MIN_NON_CHAR_QT_KEY_VALUE) {
                    if (keyCode < MAX_DEAD_QT_KEY_VALUE && keyCode > MIN_DEAD_QT_KEY_VALUE) {

                        //--------------------------------------
                        //              DEAD KEYS
                        //--------------------------------------
                        this->getDeadKeys();
                    } else {

                        //--------------------------------------
                        //            NON CHAR KEYS
                        //--------------------------------------
                        this->getNonCharKeys();
                    }
                } else {

                    //------------------------------
                    //   Custom Keys NOT EXTENDED
                    //------------------------------
                    if (!this->getCustomKeysNoExtendedKeylayoutApplied()) {
                        if (!this->getCustomKeysNoExtended()) {

                            //--------------------------
                            //   Custom Keys EXTENDED
                            //--------------------------
                            if (!this->getCustomKeysExtendedKeylayoutApplied()) {
                                if (!this->getCustomKeysExtended()) {
                                    this->showkey(keyStatusFlag, keyEvent);
                                    std::cout << "Error: Key(0x" << keyCode << ") unknown key code." << std::endl;
                                }
                            }
                        }
                    }
                }
            }
        }

        if (this->_verbose > 0) {
            this->showkey(keyStatusFlag, keyEvent);
        }
        if (this->_unvalidScanCode) {
         this->scanCode = 0;
        }
    }

};


