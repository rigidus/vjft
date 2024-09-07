// key_map.h
`#ifndef KEY_MAP_H'
`#define KEY_MAP_H'

#define KEY_MAP(X)\
define(`XY', `ifelse($4, 1, `printable($@)', `nonprintable($@)')')dnl
define(`printable',    `divert(1)`  X'($@)\`
'divert(-1)')dnl
define(`nonprintable', `divert(2)`  X'($@)\`
'divert(-1)')dnl
XY(`KEY_UNKNOWN', 0, "", 0)
XY(`KEY_ESC', 1, "\x1b", 0)
XY(`KEY_TAB', 1,       "\x09", 0)
XY(`KEY_SHIFT_TAB', 3, "\x1b[Z", 0)
XY(`KEY_BACKSPACE', 1,          "\x7f", 0)
XY(`KEY_CTRL_BACKSPACE', 1,     "\x08", 0)
XY(`KEY_ALT_BACKSPACE', 2,      "\x1b\x7f", 0)
XY(`KEY_CTRL_ALT_BACKSPACE', 2, "\x1b\x08", 0)
XY(`KEY_HOME', 3,          "\x1b[H", 0)
XY(`KEY_SHIFT_HOME', 6,    "\x1b[1;2~", 0)
XY(`KEY_ALT_HOME', 6,      "\x1b[1;3H", 0)
XY(`KEY_CTRL_HOME', 6,     "\x1b[1;5H", 0)
XY(`KEY_ALT_CTRL_HOME', 6, "\x1b[1;7H", 0)
XY(`KEY_END', 3,          "\x1b[F", 0)
XY(`KEY_ALT_END', 6,      "\x1b[1;3F", 0)
XY(`KEY_CTRL_END', 6,     "\x1b[1;5F", 0)
XY(`KEY_ALT_CTRL_END', 6, "\x1b[1;7F", 0)
XY(`KEY_INSERT', 4,     "\x1b[2~", 0)
XY(`KEY_ALT_INSERT', 6, "\x1b[2;3~", 0)
XY(`KEY_DELETE', 4,            "\x1b[3~", 0)
XY(`KEY_SHIFT_DELETE', 6,      "\x1b[3;2~", 0)
XY(`KEY_ALT_DELETE', 6,        "\x1b[3;3~", 0)
XY(`KEY_SHIFT_ALT_DELETE', 6,  "\x1b[3;4~", 0)
XY(`KEY_CTRL_DELETE', 6,       "\x1b[3;5~", 0)
XY(`KEY_SHIFT_CTRL_DELETE', 6, "\x1b[3;6~", 0)
XY(`KEY_PGUP', 4,          "\x1b[5~", 0)
XY(`KEY_ALT_PGUP', 6,      "\x1b[5;3~", 0)
XY(`KEY_ALT_CTRL_PGUP', 6, "\x1b[5;7~", 0)
XY(`KEY_PGDOWN', 4,          "\x1b[6~", 0)
XY(`KEY_ALT_PGDOWN', 6,      "\x1b[6;3~", 0)
XY(`KEY_ALT_CTRL_PGDOWN', 6, "\x1b[6;7~", 0)
XY(`KEY_UP', 3, "\x1b\x5b\x41", 0)
XY(`KEY_ALT_UP', 6, "\x1b\x5b\x31\x3b\x33\x41", 0)
XY(`KEY_SHIFT_ALT_UP', 6, "\x1b\x5b\x31\x3b\x34\x41", 0)
XY(`KEY_CTRL_UP', 6, "\x1b\x5b\x31\x3b\x35\x41", 0)
XY(`KEY_DOWN', 3, "\x1b\x5b\x42", 0)
XY(`KEY_ALT_DOWN', 6, "\x1b\x5b\x31\x3b\x33\x42", 0)
XY(`KEY_SHIFT_ALT_DOWN', 6, "\x1b\x5b\x31\x3b\x34\x42", 0)
XY(`KEY_CTRL_DOWN', 6, "\x1b\x5b\x31\x3b\x35\x42", 0)
XY(`KEY_RIGHT', 3, "\x1b\x5b\x43", 0)
XY(`KEY_ALT_RIGHT', 6, "\x1b\x5b\x31\x3b\x33\x43", 0)
XY(`KEY_SHIFT_ALT_RIGHT', 6, "\x1b\x5b\x31\x3b\x34\x43", 0)
XY(`KEY_CTRL_RIGHT', 6, "\x1b\x5b\x31\x3b\x35\x43", 0)
XY(`KEY_LEFT', 3, "\x1b\x5b\x44", 0)
XY(`KEY_ALT_LEFT', 6, "\x1b\x5b\x31\x3b\x33\x44", 0)
XY(`KEY_SHIFT_ALT_LEFT', 6, "\x1b\x5b\x31\x3b\x34\x44", 0)
XY(`KEY_CTRL_LEFT', 6, "\x1b\x5b\x31\x3b\x35\x44", 0)
XY(`KEY_ENTER', 1,     "\xa", 0)
XY(`KEY_ALT_ENTER', 2, "\x1b\xa", 0)
XY(`KEY_F1', 3,            "\x1bOP", 0)
XY(`KEY_SHIFT_F1', 6,     "\x1b[1;2P", 0)
XY(`KEY_ALT_F1', 6,       "\x1b[1;3P", 0)
XY(`KEY_SHIFT_ALT_F1', 6,  "\x1b[1;4P", 0)
XY(`KEY_CTRL_F1', 6,       "\x1b[1;5P", 0)
XY(`KEY_SHIFT_CTRL_F1', 6, "\x1b[1;6P", 0)
XY(`KEY_F2', 3, "\x1bOQ", 0)
XY(`KEY_SHIFT_F2', 6,      "\x1b[1;2Q", 0)
XY(`KEY_ALT_F2', 6,        "\x1b[1;3Q", 0)
XY(`KEY_SHIFT_ALT_F2', 6,  "\x1b[1;4Q", 0)
XY(`KEY_CTRL_F2', 6,       "\x1b[1;5Q", 0)
XY(`KEY_SHIFT_CTRL_F2', 6, "\x1b[1;6Q", 0)
XY(`KEY_F3', 3, "\x1bOR", 0)
XY(`KEY_SHIFT_F3', 6,      "\x1b[1;2R", 0)
XY(`KEY_ALT_F3', 6,        "\x1b[1;3R", 0)
XY(`KEY_SHIFT_ALT_F3', 6,  "\x1b[1;4R", 0)
XY(`KEY_CTRL_F3', 6,       "\x1b[1;5R", 0)
XY(`KEY_SHIFT_CTRL_F3', 6, "\x1b[1;6R", 0)
XY(`KEY_F4', 3, "\x1bOS", 0)
XY(`KEY_SHIFT_F4', 6,      "\x1b[1;2S", 0)
XY(`KEY_ALT_F4', 6,        "\x1b[1;3S", 0)
XY(`KEY_SHIFT_ALT_F4', 6,  "\x1b[1;4S", 0)
XY(`KEY_CTRL_F4', 6,       "\x1b[1;5S", 0)
XY(`KEY_SHIFT_CTRL_F4', 6, "\x1b[1;6S", 0)
XY(`KEY_F5', 5, "\x1b[15~", 0)
XY(`KEY_SHIFT_F5', 7,      "\x1b[15;2~", 0)
XY(`KEY_ALT_F5', 7,        "\x1b[15;3~", 0)
XY(`KEY_SHIFT_ALT_F5', 7,  "\x1b[15;4~", 0)
XY(`KEY_CTRL_F5', 7,       "\x1b[15;5~", 0)
XY(`KEY_SHIFT_CTRL_F5', 7, "\x1b[15;6~", 0)
XY(`KEY_F6', 5, "\x1b[17~", 0)
XY(`KEY_SHIFT_F6', 7,      "\x1b[17;2~", 0)
XY(`KEY_ALT_F6', 7,        "\x1b[17;3~", 0)
XY(`KEY_SHIFT_ALT_F6', 7,  "\x1b[17;4~", 0)
XY(`KEY_CTRL_F6', 7,       "\x1b[17;5~", 0)
XY(`KEY_SHIFT_CTRL_F6', 7, "\x1b[17;6~", 0)
XY(`KEY_F7', 5, "\x1b[18~", 0)
XY(`KEY_SHIFT_F7', 7,      "\x1b[18;2~", 0)
XY(`KEY_ALT_F7', 7,        "\x1b[18;3~", 0)
XY(`KEY_SHIFT_ALT_F7', 7,  "\x1b[18;4~", 0)
XY(`KEY_CTRL_F7', 7,       "\x1b[18;5~", 0)
XY(`KEY_SHIFT_CTRL_F7', 7, "\x1b[18;6~", 0)
XY(`KEY_F8', 5, "\x1b[19~", 0)
XY(`KEY_SHIFT_F8', 7,      "\x1b[19;2~", 0)
XY(`KEY_ALT_F8', 7,        "\x1b[19;3~", 0)
XY(`KEY_SHIFT_ALT_F8', 7,  "\x1b[19;4~", 0)
XY(`KEY_CTRL_F8', 7,       "\x1b[19;5~", 0)
XY(`KEY_SHIFT_CTRL_F8', 7, "\x1b[19;6~", 0)
XY(`KEY_F9', 5, "\x1b[20~", 0)
XY(`KEY_SHIFT_F9', 7,      "\x1b[20;2~", 0)
XY(`KEY_ALT_F9', 7,        "\x1b[20;3~", 0)
XY(`KEY_SHIFT_ALT_F9', 7,  "\x1b[20;4~", 0)
XY(`KEY_CTRL_F9', 7,       "\x1b[20;5~", 0)
XY(`KEY_SHIFT_CTRL_F9', 7, "\x1b[20;6~", 0)
XY(`KEY_F10', 5, "\x1b[21~", 0)
XY(`KEY_SHIFT_F10', 7,      "\x1b[21;2~", 0)
XY(`KEY_ALT_F10', 7,        "\x1b[21;3~", 0)
XY(`KEY_SHIFT_ALT_F10', 7,  "\x1b[21;4~", 0)
XY(`KEY_CTRL_F10', 7,       "\x1b[21;5~", 0)
XY(`KEY_SHIFT_CTRL_F10', 7, "\x1b[21;6~", 0)
XY(`KEY_F11', 5, "\x1b[23~", 0)
XY(`KEY_SHIFT_F11', 7,      "\x1b[23;2~", 0)
XY(`KEY_ALT_F11', 7,        "\x1b[23;3~", 0)
XY(`KEY_SHIFT_ALT_F11', 7,  "\x1b[23;4~", 0)
XY(`KEY_CTRL_F11', 7,       "\x1b[23;5~", 0)
XY(`KEY_SHIFT_CTRL_F11', 7, "\x1b[23;6~", 0)
XY(`KEY_F12', 5, "\x1b[24~", 0)
XY(`KEY_SHIFT_F12', 7,      "\x1b[24;2~", 0)
XY(`KEY_ALT_F12', 7,        "\x1b[24;3~", 0)
XY(`KEY_SHIFT_ALT_F12', 7,  "\x1b[24;4~", 0)
XY(`KEY_CTRL_F12', 7,       "\x1b[24;5~", 0)
XY(`KEY_SHIFT_CTRL_F12', 7, "\x1b[24;6~", 0)
XY(`KEY_BACKTICK', 1, "\x60", 1)
XY(`KEY_SHIFT_BACKTICK_IS_TILDE', 1, "\x7e", 1)
XY(`KEY_CTRL_BACKTICK', 1, "", 0)
XY(`KEY_SHIFT_CTRL_BACKTICK_IS_CTRL_TILDE', 1, "\x1e", 0)
XY(`KEY_SHIFT_ALT_CTRL_BACKTICK_ALT_CTRL_TILDE', 2, "\x1b\x1e", 0)
XY(`KEY_1', 1, "\x31", 1)
XY(`KEY_SHIFT_1_IS_EXCL', 1, "\x21", 1)
XY(`KEY_SHIFT_ALT_1_IS_ALT_EXCL', 2, "\x1b\x21", 0)
XY(`KEY_ALT_CTRL_1', 2, "\x1b\x31", 0)
XY(`KEY_2', 1, "\x32", 1)
XY(`KEY_SHIFT_2_IS_ATSIGN', 1, "\x40", 1)
XY(`KEY_SHIFT_ALT_2_IS_ALT_ATSIGN', 2, "\x1b\x40", 0)
XY(`KEY_SHIFT_ALT_CTRL_2_IS_ALT_CTRL_ATSIGN', 2, "\x1b\x0", 0)
XY(`KEY_3', 1, "\x33", 1)
XY(`KEY_SHIFT_3_IS_NUMBERSIGN', 1, "\x23", 1)
XY(`KEY_SHIFT_ALT_3_IS_ALT_NUMBERSIGN', 2, "\x1b\x23", 0)
XY(`KEY_4', 1, "\x34", 1)
XY(`KEY_SHIFT_4_IS_DOLLARSIGN', 1, "\x24", 1)
XY(`KEY_SHIFT_ALT_4_IS_ALT_DOLLARSIGN', 2, "\x1b\x24", 0)
XY(`KEY_5', 1, "\x35", 1)
XY(`KEY_SHIFT_5_IS_PERCENTSIGN', 1, "\x25", 1)
XY(`KEY_SHIFT_ALT_5_IS_ALT_PERCENTSIGN', 2, "\x1b\x25", 0)
XY(`KEY_6', 1, "\x36", 1)
XY(`KEY_SHIFT_6_IS_CARET', 1, "\x5e", 1)
XY(`KEY_SHIFT_ALT_6_IS_ALT_CARET', 2, "\x1b\x5e", 0)
XY(`KEY_7', 1, "\x37", 1)
XY(`KEY_SHIFT_7_IS_AMPERSAND', 1, "\x26", 1)
XY(`KEY_SHIFT_ALT_7_IS_ALT_AMPERSAND', 2, "\x1b\x26", 0)
XY(`KEY_8', 1, "\x38", 1)
XY(`KEY_SHIFT_8_IS_ASTERISK', 1, "\x2a", 1)
XY(`KEY_SHIFT_ALT_8_IS_ALT_ASTERISK', 2, "\x1b\x2a", 0)
XY(`KEY_9', 1, "\x39", 1)
XY(`KEY_SHIFT_9_IS_LEFT_PAREN', 1, "\x28", 1)
XY(`KEY_SHIFT_ALT_9_IS_ALT_LEFT_PAREN', 2, "\x1b\x28", 0)
XY(`KEY_ALT_CTRL_9', 2, "\x1b\x39", 0)
XY(`KEY_0', 1, "\x30", 1)
XY(`KEY_SHIFT_0_IS_RIGHT_PAREN', 1, "\x29", 1)
XY(`KEY_SHIFT_ALT_0_IS_ALT_RIGHT_PAREN', 2, "\x1b\x29", 0)
XY(`KEY_ALT_CTRL_0', 2, "\x1b\x30", 0)
XY(`KEY_SHIFT_MINUS_IS_UNDERSCORE', 1, "_", 1)
XY(`KEY_MINUS', 1, "-", 1)
XY(`KEY_ALT_MINUS', 2, "\x1b-", 0)
XY(`KEY_SHIFT_ALT_MINUS_IS_ALT_UNDERSCORE', 2, "\x1b_", 0)
XY(`KEY_EQ', 1, "=", 1)
XY(`KEY_SHIFT_EQ_IS_PLUS', 1, "+", 1)
XY(`KEY_ALT_EQ', 2, "\x1b=", 0)
XY(`KEY_SHIFT_ALT_EQ_IS_ALT_PLUS', 2, "\x1b+", 0)
XY(`KEY_Q', 1, "q", 1)
XY(`KEY_SHIFT_Q', 1, "Q", 1)
XY(`KEY_ALT_Q', 2, "\x1b\x71", 0)
XY(`KEY_SHIFT_ALT_Q', 2, "\x1b\x51", 0) /* C-q = DC1, C-M-Q = \1b = ESC */
XY(`KEY_CYRILLIC_SHORT_I', 2, "й", 1)
XY(`KEY_SHIFT_CYRILLIC_SHORT_I', 2, "Й", 1)
XY(`KEY_ALT_CYRILLIC_SHORT_I', 3, "\x1bй", 0)
XY(`KEY_SHIFT_ALT_CYRILLIC_SHORT_I', 3, "\x1bЙ", 0)
XY(`KEY_W', 1, "w", 1)
XY(`KEY_SHIFT_W', 1, "W", 1)
XY(`KEY_ALT_W', 2, "\x1b\x77", 0)
XY(`KEY_SHIFT_ALT_W', 2, "\x1b\x57", 0)
XY(`KEY_CTRL_W', 1, "\x17", 0)
XY(`KEY_ALT_CTRL_W', 2, "\x1b\x17", 0)
XY(`KEY_CYRILLIC_TSE', 2, "ц", 1)
XY(`KEY_SHIFT_CYRILLIC_TSE', 2, "Ц", 1)
XY(`KEY_ALT_CYRILLIC_TSE', 3, "\x1bц", 0)
XY(`KEY_ALT_SHIFT_CYRILLIC_TSE', 3, "\x1bЦ", 0)
XY(`KEY_E', 1, "e", 1)
XY(`KEY_SHIFT_E', 1, "E", 1)
XY(`KEY_ALT_E', 2, "\x1b\x65", 0)
XY(`KEY_SHIFT_ALT_E', 2, "\x1b\x45", 0)
XY(`KEY_CTRL_E', 1, "\x05", 0)
XY(`KEY_ALT_CTRL_E', 2, "\x1b\x05", 0)
XY(`KEY_CYRILLIC_U', 2, "у", 1)
XY(`KEY_SHIFT_CYRILLIC_U', 2, "У", 1)
XY(`KEY_ALT_CYRILLIC_U', 3, "\x1bу", 0)
XY(`KEY_SHIFT_ALT_CYRILLIC_U', 3, "\x1bУ", 0)
XY(`KEY_R', 1, "r", 1)
XY(`KEY_SHIFT_R', 1, "R", 1)
XY(`KEY_ALT_R', 2, "\x1b\x72", 0)
XY(`KEY_SHIFT_ALT_R', 2, "\x1b\x52", 0)
XY(`KEY_CTRL_R', 1, "\x12", 0)
XY(`KEY_ALT_CTRL_R', 2, "\x1b\x12", 0)
XY(`KEY_CYRILLIC_K', 2, "к", 1)
XY(`KEY_SHIFT_CYRILLIC_K', 2, "К", 1)
XY(`KEY_ALT_CYRILLIC_K', 3, "\x1bк", 0)
XY(`KEY_SHIFT_ALT_CYRILLIC_K', 3, "\x1bК", 0)
XY(`KEY_T', 1, "t", 1)
XY(`KEY_SHIFT_T', 1, "T", 1)
XY(`KEY_ALT_T', 2, "\x1b\x74", 0)
XY(`KEY_SHIFT_ALT_T', 2, "\x1b\x54", 0)
XY(`KEY_CTRL_T', 1, "\x14", 0)
XY(`KEY_ALT_CTRL_T', 2, "\x1b\x14", 0)
XY(`KEY_CYRILLIC_EH', 2, "е", 1)
XY(`KEY_SHIFT_CYRILLIC_EH', 2, "Е", 1)
XY(`KEY_ALT_CYRILLIC_EH', 3, "\x1bе", 0)
XY(`KEY_SHIFT_ALT_CYRILLIC_EH', 3, "\x1bЕ", 0)
XY(`KEY_Y', 1, "y", 1)
XY(`KEY_SHIFT_Y', 1, "Y", 1)
XY(`KEY_ALT_Y', 2, "\x1b\x79", 0)
XY(`KEY_SHIFT_ALT_Y', 2, "\x1b\x59", 0)
XY(`KEY_CTRL_Y', 1, "\x19", 0)
XY(`KEY_ALT_CTRL_Y', 2, "\x1b\x19", 0)
XY(`KEY_CYRILLIC_EN', 2, "н", 1)
XY(`KEY_SHIFT_CYRILLIC_EN', 2, "Н", 1)
XY(`KEY_ALT_CYRILLIC_EN', 3, "\x1bн", 0)
XY(`KEY_SHIFT_ALT_CYRILLIC_EN', 3, "\x1bН", 0)
XY(`KEY_U', 1, "u", 1)
XY(`KEY_SHIFT_U', 1, "U", 1)
XY(`KEY_ALT_U', 2, "\x1b\x75", 0)
XY(`KEY_SHIFT_ALT_U', 2, "\x1b\x55", 0)
XY(`KEY_CTRL_U', 1, "\x15", 0)
XY(`KEY_ALT_CTRL_U', 2, "\x1b\x15", 0)
XY(`KEY_CYRILLIC_GE', 2, "г", 1)
XY(`KEY_SHIFT_CYRILLIC_GE', 2, "Г", 1)
XY(`KEY_ALT_CYRILLIC_GE', 3, "\x1bг", 0)
XY(`KEY_SHIFT_ALT_CYRILLIC_GE', 3, "\x1bГ", 0)
XY(`KEY_I', 1, "i", 1)
XY(`KEY_SHIFT_I', 1, "I", 1)
XY(`KEY_ALT_I', 2, "\x1b\x69", 0)
XY(`KEY_SHIFT_ALT_I', 2, "\x1b\x49", 0)
/* Ctrl+I = Tab */
XY(`KEY_ALT_CTRL_I', 2, "\x1b\x09", 0)
XY(`KEY_CYRILLIC_SHA', 2, "ш", 1)
XY(`KEY_SHIFT_CYRILLIC_SHA', 2, "Ш", 1)
XY(`KEY_ALT_CYRILLIC_SHA', 3, "\x1bш", 0)
XY(`KEY_SHIFT_ALT_CYRILLIC_SHA', 3, "\x1bШ", 0)
XY(`KEY_O', 1, "o", 1)
XY(`KEY_SHIFT_O', 1, "O", 1)
XY(`KEY_ALT_O', 2, "\x1b\x6f", 0)
XY(`KEY_SHIFT_ALT_O', 2, "\x1b\x4f", 0) /* fixme: duplicate bug */
XY(`KEY_CTRL_O', 1, "\x0f", 0)
XY(`KEY_ALT_CTRL_O', 2, "\x1b\x0f", 0)
XY(`KEY_CYRILLIC_SHCHA', 2, "щ", 1)
XY(`KEY_SHIFT_CYRILLIC_SHCHA', 2, "Щ", 1)
XY(`KEY_ALT_CYRILLIC_SHCHA', 3, "\x1bщ", 0)
XY(`KEY_SHIFT_ALT_CYRILLIC_SHCHA', 3, "\x1bЩ", 0)
XY(`KEY_P', 1, "p", 1)
XY(`KEY_SHIFT_P', 1, "P", 1)
XY(`KEY_ALT_P', 2, "\x1b\x70", 0)
XY(`KEY_SHIFT_ALT_P', 2, "\x1b\x50", 0)
XY(`KEY_CTRL_P', 1, "\x10", 0)
XY(`KEY_ALT_CTRL_P', 2, "\x1b\x10", 0)
XY(`KEY_CYRILLIC_ZE', 2, "з", 1)
XY(`KEY_SHIFT_CYRILLIC_ZE', 2, "З", 1)
XY(`KEY_ALT_CYRILLIC_ZE', 3, "\x1bз", 0)
XY(`KEY_SHIFT_ALT_CYRILLIC_ZE', 3, "\x1bЗ", 0)
XY(`KEY_LEFT_SQUARE_BRACKET', 1, "[", 1)
XY(`KEY_SHIFT_LEFT_SQUARE_BRACKET_IS_LEFT_CURVE_BRACKET', 1, "{", 1)
XY(`KEY_ALT_LEFT_SQUARE_BRACKET', 2, "\x1b\x5b", 0) /* fixme ^[... */
XY(`KEY_SHIFT_ALT_LEFT_SQUARE_BRACKET', 2, "\x1b\x7b", 0)
/* Ctrl+[ = ESC */
XY(`KEY_ALT_CTRL_LEFT_SQUARE_BRACKET', 2, "\x1b\x1b", 0)
XY(`KEY_CYRILLIC_KHA', 2, "х", 1)
XY(`KEY_SHIFT_CYRILLIC_KHA', 2, "Х", 1)
XY(`KEY_ALT_CYRILLIC_KHA', 3, "\x1bх", 0)
XY(`KEY_SHIFT_ALT_CYRILLIC_KHA', 3, "\x1bХ", 0)
XY(`KEY_RIGHT_SQUARE_BRACKET', 1, "]", 1)
XY(`KEY_ALT_RIGHT_SQUARE_BRACKET', 2, "\x1b\x5d", 1)
XY(`KEY_SHIFT_ALT_RIGHT_SQUARE_BRACKET', 2, "\x1b\x7d", 0)
XY(`KEY_CTRL_RIGHT_SQUARE_BRACKET', 1, "\x1d", 0)
XY(`KEY_ALT_CTRL_RIGHT_SQUARE_BRACKET', 2, "\x1b\x1d", 0)
XY(`KEY_SHIFT_RIGHT_SQUARE_BRACKET_IS_RIGHT_CURVE_BRACKET', 1, "}", 0)
XY(`KEY_CYRILLIC_HARDSIGN', 2, "ъ", 1)
XY(`KEY_SHIFT_CYRILLIC_HARDSIGN', 2, "Ъ", 1)
XY(`KEY_ALT_CYRILLIC_HARDSIGN', 3, "\x1bъ", 0)
XY(`KEY_SHIFT_ALT_CYRILLIC_HARDSIGN', 3, "\x1bЪ", 0)
XY(`KEY_BACKSLASH', 1, "\\", 1)
XY(`KEY_SHIFT_BACKSLASH_IS_VERTICAL_LINE', 1, "|", 1)
XY(`KEY_ALT_BACKSLASH', 2, "\x1b\x5c", 0)
XY(`KEY_SHIFT_ALT_BACKSLASH', 2, "\x1b\x7c", 0)
/* CTRL_BACKSLASH = SIGQUIT */
/* ALT_CTRL_BACKSLASH = SIGQUIT */
XY(`KEY_A', 1, "a", 1)
XY(`KEY_SHIFT_A', 1, "A", 1)
XY(`KEY_ALT_A', 2, "\x1b\x61", 0)
XY(`KEY_SHIFT_ALT_A', 2, "\x1b\x41", 0)
XY(`KEY_CTRL_A', 1, "\x01", 0)
XY(`KEY_ALT_CTRL_A', 2, "\x1b\x01", 0)
XY(`KEY_CYRILLIC_EF', 2, "ф", 1)
XY(`KEY_SHIFT_CYRILLIC_EF', 2, "Ф", 1)
XY(`KEY_ALT_CYRILLIC_EF', 3, "\x1bф", 0)
XY(`KEY_SHIFT_ALT_CYRILLIC_EF', 3, "\x1bФ", 0)
XY(`KEY_S', 1, "s", 1)
XY(`KEY_SHIFT_S', 1, "S", 1)
XY(`KEY_ALT_S', 2, "\x1b\x73", 0)
XY(`KEY_SHIFT_ALT_S', 2, "\x1b\x53", 0) /* C-s = C-M-s = STOP_OUTPUT */
XY(`KEY_CYRILLIC_YERY', 2, "ы", 1)
XY(`KEY_SHIFT_CYRILLIC_YERY', 2, "Ы", 1)
XY(`KEY_ALT_CYRILLIC_YERY', 3, "\x1bы", 0)
XY(`KEY_SHIFT_ALT_CYRILLIC_YERY', 3, "\x1bЫ", 0)
XY(`KEY_D', 1, "d", 1)
XY(`KEY_SHIFT_D', 1, "D", 1)
XY(`KEY_ALT_D', 2, "\x1b\x64", 0)
XY(`KEY_SHIFT_ALT_D', 2, "\x1b\x44", 0)
XY(`KEY_CTRL_D', 1, "\x04", 0)
XY(`KEY_ALT_CTRL_D', 2, "\x1b\x04", 0)
XY(`KEY_CYRILLIC_VE', 2, "в", 1)
XY(`KEY_SHIFT_CYRILLIC_VE', 2, "В", 1)
XY(`KEY_ALT_CYRILLIC_VE', 3, "\x1bв", 0)
XY(`KEY_SHIFT_ALT_CYRILLIC_VE', 3, "\x1bВ", 0)
XY(`KEY_F', 1, "f", 1)
XY(`KEY_SHIFT_F', 1, "F", 1)
XY(`KEY_ALT_F', 2, "\x1b\x66", 0)
XY(`KEY_SHIFT_ALT_F', 2, "\x1b\x46", 0)
XY(`KEY_CTRL_F', 1, "\x06", 0)
XY(`KEY_ALT_CTRL_F', 2, "\x1b\x06", 0)
XY(`KEY_CYRILLIC_A', 2, "а", 1)
XY(`KEY_SHIFT_CYRILLIC_A', 2, "А", 1)
XY(`KEY_ALT_CYRILLIC_A', 3, "\x1bа", 0)
XY(`KEY_SHIFT_ALT_CYRILLIC_A', 3, "\x1bА", 0)
XY(`KEY_G', 1, "g", 1)
XY(`KEY_SHIFT_G', 1, "G", 1)
XY(`KEY_ALT_G', 2, "\x1b\x67", 0)
XY(`KEY_SHIFT_ALT_G', 2, "\x1b\x47", 0)
XY(`KEY_CTRL_G', 1, "\x07", 0)
XY(`KEY_ALT_CTRL_G', 2, "\x1b\x07", 0)
XY(`KEY_CYRILLIC_PE', 2, "п", 1)
XY(`KEY_SHIFT_CYRILLIC_PE', 2, "П", 1)
XY(`KEY_ALT_CYRILLIC_PE', 3, "\x1bп", 0)
XY(`KEY_SHIFT_ALT_CYRILLIC_PE', 3, "\x1bП", 0)
XY(`KEY_H', 1, "h", 1)
XY(`KEY_SHIFT_H', 1, "H", 1)
XY(`KEY_ALT_H', 2, "\x1b\x68", 0)
XY(`KEY_SHIFT_ALT_H', 2, "\x1b\x48", 0) /* C-h = C-BS, C-M-h = C-M-BS */
XY(`KEY_CYRILLIC_ER', 2, "р", 1)
XY(`KEY_SHIFT_CYRILLIC_ER', 2, "Р", 1)
XY(`KEY_ALT_CYRILLIC_ER', 3, "\x1bр", 0)
XY(`KEY_SHIFT_ALT_CYRILLIC_ER', 3, "\x1bР", 0)
XY(`KEY_J', 1, "j", 1)
XY(`KEY_SHIFT_J', 1, "J", 1)
XY(`KEY_ALT_J', 2, "\x1b\x67", 0)
XY(`KEY_SHIFT_ALT_J', 2, "\x1b\x4a", 0) /* C-j = C-Enter, C-M-j = M-Enter */
XY(`KEY_CYRILLIC_O', 2, "о", 1)
XY(`KEY_SHIFT_CYRILLIC_O', 2, "О", 1)
XY(`KEY_ALT_CYRILLIC_O', 3, "\x1bо", 0)
XY(`KEY_SHIFT_ALT_CYRILLIC_O', 3, "\x1bО", 0)
XY(`KEY_K', 1, "k", 1)
XY(`KEY_SHIFT_K', 1, "K", 1)
XY(`KEY_ALT_K', 2, "\x1b\x6b", 0)
XY(`KEY_SHIFT_ALT_K', 2, "\x1b\x4b", 0)
XY(`KEY_CTRL_K', 1, "\x0b", 0)
XY(`KEY_ALT_CTRL_K', 2, "\x1b\x0b", 0)
XY(`KEY_CYRILLIC_EL', 2, "л", 1)
XY(`KEY_SHIFT_CYRILLIC_EL', 2, "Л", 1)
XY(`KEY_ALT_CYRILLIC_EL', 3, "\x1bл", 0)
XY(`KEY_SHIFT_ALT_CYRILLIC_EL', 3, "\x1bЛ", 0)
XY(`KEY_L', 1, "l", 1)
XY(`KEY_SHIFT_L', 1, "L", 1)
XY(`KEY_ALT_L', 2, "\x1b\x6c", 0)
XY(`KEY_SHIFT_ALT_L', 2, "\x1b\x4c", 0)
XY(`KEY_CTRL_L', 1, "\x0c", 0)
XY(`KEY_ALT_CTRL_L', 2, "\x1b\x0c", 0)
XY(`KEY_CYRILLIC_DE', 2, "д", 1)
XY(`KEY_SHIFT_CYRILLIC_DE', 2, "Д", 1)
XY(`KEY_ALT_CYRILLIC_DE', 3, "\x1bд", 0)
XY(`KEY_SHIFT_ALT_CYRILLIC_DE', 3, "\x1bД", 0)
XY(`KEY_SEMICOLON', 1, ";", 1)
XY(`KEY_SHIFT_SEMICOLON_IS_COLON', 1, ":", 1)
XY(`KEY_ALT_SEMICOLON', 2, "\x1b\x3b", 0)
XY(`KEY_SHIFT_ALT_SEMICOLON', 2, "\x1b\x3a", 0)
XY(`KEY_CTRL_SEMICOLON', 1, "\x0c", 0)
XY(`KEY_ALT_CTRL_SEMICOLON', 2, "\x1b\x0c", 0)
XY(`KEY_CYRILLIC_ZHE', 2, "ж", 1)
XY(`KEY_SHIFT_CYRILLIC_ZHE', 2, "Ж", 1)
XY(`KEY_ALT_CYRILLIC_ZHE', 3, "\x1bж", 0)
XY(`KEY_SHIFT_ALT_CYRILLIC_ZHE', 3, "\x1bЖ", 0)
XY(`KEY_TICK', 1, "\047", 1)
XY(`KEY_SHIFT_TICK_IS_QUOTATIONS', 1, "\"", 1)
XY(`KEY_ALT_TICK', 2, "\x1b\x27", 0)
XY(`KEY_SHIFT_ALT_TICK', 2, "\x1b\x22", 0) /* no C- & C-M- */
XY(`KEY_CYRILLIC_E', 2, "э", 1)
XY(`KEY_SHIFT_CYRILLIC_E', 2, "Э", 1)
XY(`KEY_ALT_CYRILLIC_E', 3, "\x1bэ", 0)
XY(`KEY_SHIFT_ALT_CYRILLIC_E', 3, "\x1bЭ", 0)
XY(`KEY_Z', 1, "z", 1)
XY(`KEY_SHIFT_Z', 1, "Z", 1)
XY(`KEY_CYRILLIC_YA', 2, "я", 1)
XY(`KEY_SHIFT_CYRILLIC_YA', 2, "Я", 1)
XY(`KEY_ALT_CYRILLIC_YA', 3, "\x1bя", 0)
XY(`KEY_SHIFT_ALT_CYRILLIC_YA', 3, "\x1bЯ", 0)
XY(`KEY_X', 1, "x", 1)
XY(`KEY_SHIFT_X', 1, "X", 1)
XY(`KEY_ALT_X', 2, "\x1bx", 0)
XY(`KEY_SHIFT_ALT_X', 2, "\x1bX", 0)
XY(`KEY_CTRL_X', 1, "\x18", 0)
XY(`KEY_ALT_CTRL_X', 2, "\x1b\x18", 0)
XY(`KEY_CYRILLIC_CHE', 2, "ч", 1)
XY(`KEY_SHIFT_CYRILLIC_CHE', 2, "Ч", 1)
XY(`KEY_ALT_CYRILLIC_CHE', 3, "\x1bч", 0)
XY(`KEY_SHIFT_ALT_CYRILLIC_CHE', 3, "\x1bЧ", 0)
XY(`KEY_C', 1, "c", 1)
XY(`KEY_SHIFT_C', 1, "C", 1)
XY(`KEY_CYRILLIC_ES', 2, "с", 1)
XY(`KEY_SHIFT_CYRILLIC_ES', 2, "С", 1)
XY(`KEY_ALT_CYRILLIC_ES', 3, "\x1bс", 0)
XY(`KEY_SHIFT_ALT_CYRILLIC_ES', 3, "\x1bС", 0)
XY(`KEY_V', 1, "v", 1)
XY(`KEY_SHIFT_V', 1, "V", 1)
XY(`KEY_ALT_V', 2, "\x1bv", 0)
XY(`KEY_SHIFT_ALT_V', 2, "\x1bV", 0)
XY(`KEY_CTRL_V', 1, "\x16", 0)
XY(`KEY_ALT_CTRL_V', 2, "\x1b\x16", 0)
XY(`KEY_CYRILLIC_EM', 2, "м", 1)
XY(`KEY_SHIFT_CYRILLIC_EM', 2, "М", 1)
XY(`KEY_ALT_CYRILLIC_EM', 3, "\x1bм", 0)
XY(`KEY_SHIFT_ALT_CYRILLIC_EM', 3, "\x1bМ", 0)
XY(`KEY_B', 1, "b", 1)
XY(`KEY_SHIFT_B', 1, "B", 1)
XY(`KEY_ALT_B', 2, "\x1b\x62", 0)
XY(`KEY_SHIFT_ALT_B', 2, "\x1b\x42", 0)
XY(`KEY_CTRL_B', 1, "\x02", 0)
XY(`KEY_ALT_CTRL_B', 2, "\x1b\x02", 0)
XY(`KEY_CYRILLIC_I', 2, "и", 1)
XY(`KEY_SHIFT_CYRILLIC_I', 2, "И", 1)
XY(`KEY_ALT_CYRILLIC_I', 3, "\x1bи", 0)
XY(`KEY_SHIFT_ALT_CYRILLIC_I', 3, "\x1bИ", 0)
XY(`KEY_N', 1, "n", 1)
XY(`KEY_SHIFT_N', 1, "N", 1)
XY(`KEY_ALT_N', 2, "\x1b\x6e", 0)
XY(`KEY_SHIFT_ALT_N', 2, "\x1b\x4e", 0)
XY(`KEY_CTRL_N', 1, "\x0e", 0)
XY(`KEY_ALT_CTRL_N', 2, "\x1b\x0e", 0)
XY(`KEY_CYRILLIC_TE', 2, "т", 1)
XY(`KEY_SHIFT_CYRILLIC_TE', 2, "Т", 1)
XY(`KEY_ALT_CYRILLIC_TE', 3, "\x1bт", 0)
XY(`KEY_SHIFT_ALT_CYRILLIC_TE', 3, "\x1bТ", 0)
XY(`KEY_M', 1, "m", 1)
XY(`KEY_SHIFT_M', 1, "M", 1)
XY(`KEY_ALT_M', 2, "\x1b\x6d", 0)
XY(`KEY_SHIFT_ALT_M', 2, "\x1b\x4d", 0) /* C-m = Enter, C-M-m = M-Enter*/
XY(`KEY_CYRILLIC_SOFTSIGN', 2, "ь", 1)
XY(`KEY_SHIFT_CYRILLIC_SOFTSIGN', 2, "Ь", 1)
XY(`KEY_ALT_CYRILLIC_SOFTSIGN', 3, "\x1bь", 0)
XY(`KEY_SHIFT_ALT_CYRILLIC_SOFTSIGN', 3, "\x1bЬ", 0)
XY(`KEY_COLON', 1, ",", 1)
XY(`KEY_SHIFT_COMMA_IS_LESS ', 1, "<", 1)
XY(`KEY_ALT_COLON', 2, "\x1b\x2c", 0)
XY(`KEY_SHIFT_ALT_COLON', 2, "\x1b\x3c", 0) /* not C- & C-M */
XY(`KEY_CYRILLIC_BE ', 2, "б", 1)
XY(`KEY_SHIFT_CYRILLIC_BE ', 2, "Б", 1)
XY(`KEY_ALT_CYRILLIC_BE ', 3, "\x1bб", 0)
XY(`KEY_SHIFT_ALT_CYRILLIC_BE ', 3, "\x1bБ", 0)
XY(`KEY_DOT', 1, ".", 1)
XY(`KEY_SHIFT_DOT_IS_GREATER', 1, ">", 1)
XY(`KEY_ALT_DOT', 2, "\x1b\x2e", 0)
XY(`KEY_SHIFT_ALT_DOT', 2, "\x1b\x3e", 0) /* not C- & C-M */
XY(`KEY_CYRILLIC_YU', 2, "ю", 1)
XY(`KEY_SHIFT_CYRILLIC_YU', 2, "Ю", 1)
XY(`KEY_ALT_CYRILLIC_YU', 3, "\x1bю", 0)
XY(`KEY_SHIFT_ALT_CYRILLIC_YU', 3, "\x1bЮ", 0)
XY(`KEY_SLASH', 1, "/", 1)
XY(`KEY_SHIFT_SLASH_IS_QUESTION', 1, "?", 1)
XY(`KEY_ALT_SLASH', 2, "\x1b\x2f", 0)
XY(`KEY_SHIFT_ALT_SLASH', 2, "\x1b\x3d", 0) /* not C- & C-M */
XY(`KEY_SPACE', 1, "\x20", 1)
XY(`KEY_SHIFT_ALT_SPACE', 2, "\x1b\x20", 0)
divert(0)dnl
undivert(1)dnl
undivert(2)dnl
divert(0)
`#endif'
divert(-1)dnl

/*

Для просмотра escape-последовательностей, генерируемых нажатиями
клавиш в вашем терминале, вы можете использовать несколько
инструментов или методов, в зависимости от вашей операционной
системы и настроек. Вот несколько популярных способов:

1. Использование showkey на Linux

showkey может быть запущен в двух режимах: ascii и keycode. Для
просмотра escape-последовательностей следует использовать режим ascii.

showkey -a

Запустите эту команду в терминале, и она будет показывать ASCII коды
нажатых клавиш. Для выхода из showkey обычно нужно подождать 10 секунд
без каких-либо нажатий клавиш.

2. Использование cat или xxd

Можно использовать простую команду cat в режиме вывода стандартного
ввода на экран:

cat

После запуска команды начинайте нажимать клавиши. Нажатие, например,
Ctrl-V перед специальной клавишей заставит cat печатать её литеральное
представление. Для выхода используйте Ctrl-D.

Если доступна утилита xxd, это также может быть полезный инструмент
для визуализации ввода:

cat | xxd

это позволит увидеть ввод в шестнадцатеричном и символьном формате.

*/

/*

Соответствие между Ctrl-символами и их обозначениями заключается в том,
что ^char — это символ, код которого на 64 меньше, чем char . В двоичном формате
это имеет смысл: символы — это символы, номер которых записан 0 1 0vwxyz, а
соответствующий управляющий символ - это тот, номер которого  0 0 0vwxyz.
Для ^? перевернутый бит тот же, но он установлен, а не очищен: ? это 0 0 111111
и ^? - 0 1 111111.

*/

/*
C-c отправляет SIGINT
С-4 или C-\ отправляет SIGQUIT

Вы всегда можете вводить непечатаемые символы в bash с помощью "C-v, key".
Ctrl-V сообщает терминалу не интерпретировать следующий символ.

https://unix.stackexchange.com/questions/226327/what-does-ctrl4-and-ctrl-do-in-bash/226333#226333

*/

/*

В те времена, когда серьезные компьютеры были размером с несколько вертикальных
холодильников, терминал связывался с центральным компьютером по последовательному
кабелю, используя только символы и символы. Символы были частью некоторого
стандартизированного набора символов, например ASCII или EBCDIC, но обычно ASCII.

ASCII имеет 33 управляющих символа, и оператор терминала отправлял их, нажимая
специальную клавишу (например, DEL) или удерживая клавишу CTRL и нажимая
другую клавишу. Центральный компьютер видел только полученный управляющий
символ; он не знал, какие клавиши были нажаты для создания символа.

Программа эмуляции терминала, такая как xterm, имитирует это поведение. Эмулятор
терминала предоставляет возможность отправить все 33 управляющих символа ASCII

Эмуляторы терминала обычно используют следующие сопоставления для генерации
управляющих символов:

keypress       ASCII
--------------------
ESCAPE          27
DELETE          127
BACKSPACE       8
CTRL+SPACE      0
CTRL+@          0
CTRL+A          1
CTRL+B          2
CTRL+C          3
etc...
CTRL+X          24
CTRL+Y          25
CTRL+Z          26
CTRL+[          27
CTRL+\          28
CTRL+]          29
CTRL+^          30
CTRL+_          31

см также: https://unix.stackexchange.com/questions/79374/are-there-any-linux-terminals-which-can-handle-all-key-combinations/79561#79561

https://unix.stackexchange.com/questions/116629/how-do-keyboard-input-and-text-output-work/116630#116630

https://unix.stackexchange.com/questions/116629/how-do-keyboard-input-and-text-output-work/116630#116630

*/

/*

Порядок модификаторов:
- SHIFT
- ALT
- SHIFT_ALT
- CTRL
- SHIFT_CTRL
- ALT_CTRL
- SHIFT_ALT_CTRL

*/
