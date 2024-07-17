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
    X(KEY_UNKNOWN, "")                          \
    X(KEY_ESC, "\x1b")                          \
    X(KEY_TAB,       "\x09")                    \
    X(KEY_SHIFT_TAB, "\x1b[Z")                  \
    X(KEY_BACKSPACE,          "\x7f")           \
    X(KEY_CTRL_BACKSPACE,     "\x08")           \
    X(KEY_ALT_BACKSPACE,      "\x1b\x7f")       \
    X(KEY_CTRL_ALT_BACKSPACE, "\x1b\x08")       \
    X(KEY_HOME,          "\x1b[H")              \
    X(KEY_SHIFT_HOME,    "\x1b[1;2~")           \
    X(KEY_ALT_HOME,      "\x1b[1;3H")           \
    X(KEY_CTRL_HOME,     "\x1b[1;5H")           \
    X(KEY_ALT_CTRL_HOME, "\x1b[1;7H")           \
    X(KEY_END,          "\x1b[F")               \
    X(KEY_ALT_END,      "\x1b[1;3F")            \
    X(KEY_CTRL_END,     "\x1b[1;5F")            \
    X(KEY_ALT_CTRL_END, "\x1b[1;7F")            \
    X(KEY_INSERT,     "\x1b[2~")                \
    X(KEY_ALT_INSERT, "\x1b[2;3~")              \
    X(KEY_DELETE,            "\x1b[3~")         \
    X(KEY_SHIFT_DELETE,      "\x1b[3;2~")       \
    X(KEY_ALT_DELETE,        "\x1b[3;3~")       \
    X(KEY_SHIFT_ALT_DELETE,  "\x1b[3;4~")       \
    X(KEY_CTRL_DELETE,       "\x1b[3;5~")       \
    X(KEY_SHIFT_CTRL_DELETE, "\x1b[3;6~")       \
    X(KEY_PGUP,          "\x1b[5~")             \
    X(KEY_ALT_PGUP,      "\x1b[5;3~")           \
    X(KEY_ALT_CTRL_PGUP, "\x1b[5;7~")           \
    X(KEY_PGDOWN,          "\x1b[6~")           \
    X(KEY_ALT_PGDOWN,      "\x1b[6;3~")         \
    X(KEY_ALT_CTRL_PGDOWN, "\x1b[6;7~")         \
    X(KEY_F1,            "\x1bOP")              \
    X(KEY_SHIFT_F1,      "\x1b[1;2P")           \
    X(KEY_ALT_F1,        "\x1b[1;3P")           \
    X(KEY_SHIFT_ALT_F1,  "\x1b[1;4P")           \
    X(KEY_CTRL_F1,       "\x1b[1;5P")           \
    X(KEY_SHIFT_CTRL_F1, "\x1b[1;6P")           \
    X(KEY_F2, "\x1bOQ")                         \
    X(KEY_SHIFT_F2,      "\x1b[1;2Q")           \
    X(KEY_ALT_F2,        "\x1b[1;3Q")           \
    X(KEY_SHIFT_ALT_F2,  "\x1b[1;4Q")           \
    X(KEY_CTRL_F2,       "\x1b[1;5Q")           \
    X(KEY_SHIFT_CTRL_F2, "\x1b[1;6Q")           \
    X(KEY_F3, "\x1bOR")                         \
    X(KEY_SHIFT_F3,      "\x1b[1;2R")           \
    X(KEY_ALT_F3,        "\x1b[1;3R")           \
    X(KEY_SHIFT_ALT_F3,  "\x1b[1;4R")           \
    X(KEY_CTRL_F3,       "\x1b[1;5R")           \
    X(KEY_SHIFT_CTRL_F3, "\x1b[1;6R")           \
    X(KEY_F4, "\x1bOS")                         \
    X(KEY_SHIFT_F4,      "\x1b[1;2S")           \
    X(KEY_ALT_F4,        "\x1b[1;3S")           \
    X(KEY_SHIFT_ALT_F4,  "\x1b[1;4S")           \
    X(KEY_CTRL_F4,       "\x1b[1;5S")           \
    X(KEY_SHIFT_CTRL_F4, "\x1b[1;6S")           \
    X(KEY_F5, "\x1b[15~")                       \
    X(KEY_SHIFT_F5,      "\x1b[15;2~")          \
    X(KEY_ALT_F5,        "\x1b[15;3~")          \
    X(KEY_SHIFT_ALT_F5,  "\x1b[15;4~")          \
    X(KEY_CTRL_F5,       "\x1b[15;5~")          \
    X(KEY_SHIFT_CTRL_F5, "\x1b[15;6~")          \
    X(KEY_F6, "\x1b[17~")                       \
    X(KEY_SHIFT_F6,      "\x1b[17;2~")          \
    X(KEY_ALT_F6,        "\x1b[17;3~")          \
    X(KEY_SHIFT_ALT_F6,  "\x1b[17;4~")          \
    X(KEY_CTRL_F6,       "\x1b[17;5~")          \
    X(KEY_SHIFT_CTRL_F6, "\x1b[17;6~")          \
    X(KEY_F7, "\x1b[18~")                       \
    X(KEY_SHIFT_F7,      "\x1b[18;2~")          \
    X(KEY_ALT_F7,        "\x1b[18;3~")          \
    X(KEY_SHIFT_ALT_F7,  "\x1b[18;4~")          \
    X(KEY_CTRL_F7,       "\x1b[18;5~")          \
    X(KEY_SHIFT_CTRL_F7, "\x1b[18;6~")          \
    X(KEY_F8, "\x1b[19~")                       \
    X(KEY_SHIFT_F8,      "\x1b[19;2~")          \
    X(KEY_ALT_F8,        "\x1b[19;3~")          \
    X(KEY_SHIFT_ALT_F8,  "\x1b[19;4~")          \
    X(KEY_CTRL_F8,       "\x1b[19;5~")          \
    X(KEY_SHIFT_CTRL_F8, "\x1b[19;6~")          \
    X(KEY_F9, "\x1b[20~")                       \
    X(KEY_SHIFT_F9,      "\x1b[20;2~")          \
    X(KEY_ALT_F9,        "\x1b[20;3~")          \
    X(KEY_SHIFT_ALT_F9,  "\x1b[20;4~")          \
    X(KEY_CTRL_F9,       "\x1b[20;5~")          \
    X(KEY_SHIFT_CTRL_F9, "\x1b[20;6~")          \
    X(KEY_F10, "\x1b[21~")                      \
    X(KEY_SHIFT_F10,      "\x1b[21;2~")         \
    X(KEY_ALT_F10,        "\x1b[21;3~")         \
    X(KEY_SHIFT_ALT_F10,  "\x1b[21;4~")         \
    X(KEY_CTRL_F10,       "\x1b[21;5~")         \
    X(KEY_SHIFT_CTRL_F10, "\x1b[21;6~")         \
    X(KEY_F11, "\x1b[23~")                      \
    X(KEY_SHIFT_F11,      "\x1b[23;2~")         \
    X(KEY_ALT_F11,        "\x1b[23;3~")         \
    X(KEY_SHIFT_ALT_F11,  "\x1b[23;4~")         \
    X(KEY_CTRL_F11,       "\x1b[23;5~")         \
    X(KEY_SHIFT_CTRL_F11, "\x1b[23;6~")         \
    X(KEY_F12, "\x1b[24~")                      \
    X(KEY_SHIFT_F12,      "\x1b[24;2~")         \
    X(KEY_ALT_F12,        "\x1b[24;3~")         \
    X(KEY_SHIFT_ALT_F12,  "\x1b[24;4~")         \
    X(KEY_CTRL_F12,       "\x1b[24;5~")         \
    X(KEY_SHIFT_CTRL_F12, "\x1b[24;6~")
