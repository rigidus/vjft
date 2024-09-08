// cmd.h

#ifndef CMD_H
#define CMD_H

#include "ctrlstk.h"
#include "msg.h"

extern MessageList messageList;
extern void connect_to_server(const char* server_ip, int port);

typedef void (*CmdFunc)(MessageNode*, const char* param);

void cmd_connect();
void cmd_enter(MessageNode* msg, const char* param);
void cmd_alt_enter(MessageNode* msg, const char* param);
void cmd_backspace(MessageNode* msg, const char* param);
void cmd_prev_msg();
void cmd_next_msg();
void cmd_backward_char();
void cmd_forward_char();
void cmd_forward_word();
void cmd_backward_word();
void cmd_to_end_of_line();
void cmd_to_beginning_of_line();
void cmd_insert();
void cmd_copy(MessageNode* node, const char* param);
void cmd_cut(MessageNode* node, const char* param);
void cmd_paste(MessageNode* node, const char* param);
void cmd_toggle_cursor_shadow(MessageNode* node, const char* param);
void cmd_undo(MessageNode* msg, const char* param);
void cmd_redo(MessageNode* msg, const char* param);

#endif
