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

Порядок модификаторов:
- SHIFT
- ALT
- SHIFT_ALT
- CTRL
- SHIFT_CTRL
- ALT_CTRL
- SHIFT_ALT_CTRL

*/


// Список клавиатурных входов
#define KEY_MAP(X)                              \
    X(KEY_UNKNOWN, 0, "")                       \
    X(KEY_ESC, 1, "\x1b")                       \
    X(KEY_TAB, 1,       "\x09")                 \
    X(KEY_SHIFT_TAB, 3, "\x1b[Z")               \
    X(KEY_BACKSPACE, 1,          "\x7f")        \
    X(KEY_CTRL_BACKSPACE, 1,     "\x08")        \
    X(KEY_ALT_BACKSPACE, 2,      "\x1b\x7f")    \
    X(KEY_CTRL_ALT_BACKSPACE, 2, "\x1b\x08")    \
    X(KEY_HOME, 3,          "\x1b[H")           \
    X(KEY_SHIFT_HOME, 6,    "\x1b[1;2~")        \
    X(KEY_ALT_HOME, 6,      "\x1b[1;3H")        \
    X(KEY_CTRL_HOME, 6,     "\x1b[1;5H")        \
    X(KEY_ALT_CTRL_HOME, 6, "\x1b[1;7H")        \
    X(KEY_END, 3,          "\x1b[F")            \
    X(KEY_ALT_END, 6,      "\x1b[1;3F")         \
    X(KEY_CTRL_END, 6,     "\x1b[1;5F")         \
    X(KEY_ALT_CTRL_END, 6, "\x1b[1;7F")         \
    X(KEY_INSERT, 4,     "\x1b[2~")             \
    X(KEY_ALT_INSERT, 6, "\x1b[2;3~")           \
    X(KEY_DELETE, 4,            "\x1b[3~")      \
    X(KEY_SHIFT_DELETE, 6,      "\x1b[3;2~")    \
    X(KEY_ALT_DELETE, 6,        "\x1b[3;3~")    \
    X(KEY_SHIFT_ALT_DELETE, 6,  "\x1b[3;4~")    \
    X(KEY_CTRL_DELETE, 6,       "\x1b[3;5~")    \
    X(KEY_SHIFT_CTRL_DELETE, 6, "\x1b[3;6~")    \
    X(KEY_PGUP, 4,          "\x1b[5~")          \
    X(KEY_ALT_PGUP, 6,      "\x1b[5;3~")        \
    X(KEY_ALT_CTRL_PGUP, 6, "\x1b[5;7~")        \
    X(KEY_PGDOWN, 4,          "\x1b[6~")        \
    X(KEY_ALT_PGDOWN, 6,      "\x1b[6;3~")      \
    X(KEY_ALT_CTRL_PGDOWN, 6, "\x1b[6;7~")      \
    X(KEY_ENTER, 1,     "\xa")                  \
    X(KEY_ALT_ENTER, 2, "\x1b\xa")              \
    X(KEY_F1, 3,            "\x1bOP")           \
    X(KEY_SHIFT_F1, 6,     "\x1b[1;2P")         \
    X(KEY_ALT_F1, 6,       "\x1b[1;3P")         \
    X(KEY_SHIFT_ALT_F1, 6,  "\x1b[1;4P")        \
    X(KEY_CTRL_F1, 6,       "\x1b[1;5P")        \
    X(KEY_SHIFT_CTRL_F1, 6, "\x1b[1;6P")        \
    X(KEY_F2, 3, "\x1bOQ")                      \
    X(KEY_SHIFT_F2, 6,      "\x1b[1;2Q")        \
    X(KEY_ALT_F2, 6,        "\x1b[1;3Q")        \
    X(KEY_SHIFT_ALT_F2, 6,  "\x1b[1;4Q")        \
    X(KEY_CTRL_F2, 6,       "\x1b[1;5Q")        \
    X(KEY_SHIFT_CTRL_F2, 6, "\x1b[1;6Q")        \
    X(KEY_F3, 3, "\x1bOR")                      \
    X(KEY_SHIFT_F3, 6,      "\x1b[1;2R")        \
    X(KEY_ALT_F3, 6,        "\x1b[1;3R")        \
    X(KEY_SHIFT_ALT_F3, 6,  "\x1b[1;4R")        \
    X(KEY_CTRL_F3, 6,       "\x1b[1;5R")        \
    X(KEY_SHIFT_CTRL_F3, 6, "\x1b[1;6R")        \
    X(KEY_F4, 3, "\x1bOS")                      \
    X(KEY_SHIFT_F4, 6,      "\x1b[1;2S")        \
    X(KEY_ALT_F4, 6,        "\x1b[1;3S")        \
    X(KEY_SHIFT_ALT_F4, 6,  "\x1b[1;4S")        \
    X(KEY_CTRL_F4, 6,       "\x1b[1;5S")        \
    X(KEY_SHIFT_CTRL_F4, 6, "\x1b[1;6S")        \
    X(KEY_F5, 5, "\x1b[15~")                    \
    X(KEY_SHIFT_F5, 7,      "\x1b[15;2~")       \
    X(KEY_ALT_F5, 7,        "\x1b[15;3~")       \
    X(KEY_SHIFT_ALT_F5, 7,  "\x1b[15;4~")       \
    X(KEY_CTRL_F5, 7,       "\x1b[15;5~")       \
    X(KEY_SHIFT_CTRL_F5, 7, "\x1b[15;6~")       \
    X(KEY_F6, 5, "\x1b[17~")                    \
    X(KEY_SHIFT_F6, 7,      "\x1b[17;2~")       \
    X(KEY_ALT_F6, 7,        "\x1b[17;3~")       \
    X(KEY_SHIFT_ALT_F6, 7,  "\x1b[17;4~")       \
    X(KEY_CTRL_F6, 7,       "\x1b[17;5~")       \
    X(KEY_SHIFT_CTRL_F6, 7, "\x1b[17;6~")       \
    X(KEY_F7, 5, "\x1b[18~")                    \
    X(KEY_SHIFT_F7, 7,      "\x1b[18;2~")       \
    X(KEY_ALT_F7, 7,        "\x1b[18;3~")       \
    X(KEY_SHIFT_ALT_F7, 7,  "\x1b[18;4~")       \
    X(KEY_CTRL_F7, 7,       "\x1b[18;5~")       \
    X(KEY_SHIFT_CTRL_F7, 7, "\x1b[18;6~")       \
    X(KEY_F8, 5, "\x1b[19~")                    \
    X(KEY_SHIFT_F8, 7,      "\x1b[19;2~")       \
    X(KEY_ALT_F8, 7,        "\x1b[19;3~")       \
    X(KEY_SHIFT_ALT_F8, 7,  "\x1b[19;4~")       \
    X(KEY_CTRL_F8, 7,       "\x1b[19;5~")       \
    X(KEY_SHIFT_CTRL_F8, 7, "\x1b[19;6~")       \
    X(KEY_F9, 5, "\x1b[20~")                    \
    X(KEY_SHIFT_F9, 7,      "\x1b[20;2~")       \
    X(KEY_ALT_F9, 7,        "\x1b[20;3~")       \
    X(KEY_SHIFT_ALT_F9, 7,  "\x1b[20;4~")       \
    X(KEY_CTRL_F9, 7,       "\x1b[20;5~")       \
    X(KEY_SHIFT_CTRL_F9, 7, "\x1b[20;6~")       \
    X(KEY_F10, 5, "\x1b[21~")                   \
    X(KEY_SHIFT_F10, 7,      "\x1b[21;2~")      \
    X(KEY_ALT_F10, 7,        "\x1b[21;3~")      \
    X(KEY_SHIFT_ALT_F10, 7,  "\x1b[21;4~")      \
    X(KEY_CTRL_F10, 7,       "\x1b[21;5~")      \
    X(KEY_SHIFT_CTRL_F10, 7, "\x1b[21;6~")      \
    X(KEY_F11, 5, "\x1b[23~")                   \
    X(KEY_SHIFT_F11, 7,      "\x1b[23;2~")      \
    X(KEY_ALT_F11, 7,        "\x1b[23;3~")      \
    X(KEY_SHIFT_ALT_F11, 7,  "\x1b[23;4~")      \
    X(KEY_CTRL_F11, 7,       "\x1b[23;5~")      \
    X(KEY_SHIFT_CTRL_F11, 7, "\x1b[23;6~")      \
    X(KEY_F12, 5, "\x1b[24~")                   \
    X(KEY_SHIFT_F12, 7,      "\x1b[24;2~")      \
    X(KEY_ALT_F12, 7,        "\x1b[24;3~")      \
    X(KEY_SHIFT_ALT_F12, 7,  "\x1b[24;4~")      \
    X(KEY_CTRL_F12, 7,       "\x1b[24;5~")      \
    X(KEY_SHIFT_CTRL_F12, 7, "\x1b[24;6~")      \
    X(KEY_BACKTICK, 1, "\x60")                                     \
    X(KEY_SHIFT_BACKTICK_IS_TILDE, 1, "\x7e")                      \
    X(KEY_CTRL_BACKTICK, 1, "")                                    \
    X(KEY_SHIFT_CTRL_BACKTICK_IS_CTRL_TILDE, 1, "\x1e")            \
    X(KEY_SHIFT_ALT_CTRL_BACKTICK_ALT_CTRL_TILDE, 2, "\x1b\x1e")   \


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
