/*
    Copyright 2014-2018 Mark P Jones, Portland State University

    This file is part of CEMLaBS/LLP Demos and Lab Exercises.

    CEMLaBS/LLP Demos and Lab Exercises is free software: you can
    redistribute it and/or modify it under the terms of the GNU General
    Public License as published by the Free Software Foundation, either
    version 3 of the License, or (at your option) any later version.

    CEMLaBS/LLP Demos and Lab Exercises is distributed in the hope that
    it will be useful, but WITHOUT ANY WARRANTY; without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CEMLaBS/LLP Demos and Lab Exercises.  If not, see
    <https://www.gnu.org/licenses/>.
*/
#ifndef L4_IPC_H
#define L4_IPC_H
#include <l4/types.h>
#include <l4/thread.h>
#include <l4/message.h>
//#include <l4io.h>

/* Message Tags: -----------------------------------------------------------*/

typedef struct {
  L4_Word_t raw;
} L4_MsgTag_t;

#define L4_Niltag  ((L4_MsgTag_t){raw:0})

static inline L4_Bool_t L4_IsMsgTagEqual(const L4_MsgTag_t l,
                                         const L4_MsgTag_t r) {
  return l.raw == r.raw;
}

static inline L4_Bool_t L4_IsMsgTagNotEqual(const L4_MsgTag_t l,
                                            const L4_MsgTag_t r) {
  return l.raw != r.raw;
}

static inline L4_Word_t L4_Label(L4_MsgTag_t t) {
  return (t.raw>>16) & 0xffff;
}

static inline L4_Word_t L4_UntypedWords(L4_MsgTag_t t) {
  return t.raw & 0x3f;
}

static inline L4_Word_t L4_TypedWords(L4_MsgTag_t t) {
  return (t.raw>>6) & 0x3f;
}

static inline L4_MsgTag_t L4_MsgTagAddLabel(L4_MsgTag_t t, L4_Word_t label) {
  t.raw = (label << 16) | (t.raw & 0xffff);
  return t;
}

static inline L4_MsgTag_t L4_MsgTagAddLabelTo(L4_MsgTag_t* t,
                                              L4_Word_t label) {
  t->raw = (label << 16) | (t->raw & 0xffff);
  return *t;
}

static inline L4_MsgTag_t L4_MsgTag() {
  L4_MsgTag_t tag;
  L4_StoreMR(0, &(tag.raw));
  return tag;
}

static inline void L4_Set_MsgTag(L4_MsgTag_t tag) {
  L4_LoadMR(0, tag.raw);
}

/* Message Tags: (C++ Bindings) --------------------------------------------*/

#if defined(__cplusplus)

static inline L4_Bool_t operator==(const L4_MsgTag_t& l,
                                   const L4_MsgTag_t& r) {
  return l.raw == r.raw;
}

static inline L4_Bool_t operator!=(const L4_MsgTag_t& l,
                                   const L4_MsgTag_t& r) {
  return l.raw != r.raw;
}

static inline L4_MsgTag_t operator+(L4_MsgTag_t& t, L4_Word_t label) {
  return L4_MsgTagAddLabel(t, label);
}

static inline L4_MsgTag_t operator+=(L4_MsgTag_t& t, L4_Word_t label) {
  return L4_MsgTagAddLabelTo(&t, label);
}

#endif

/* MapItems: ---------------------------------------------------------------*/

typedef struct {
    L4_Word_t raw[2];
} L4_MapItem_t;

static inline L4_MapItem_t L4_MapItem(L4_Fpage_t f, L4_Word_t sndBase) {
  L4_MapItem_t mapItem;
  mapItem.raw[0] = (sndBase & ~0x3ff) | 0x8;
  mapItem.raw[1] = f.raw;
  return mapItem;
}

static inline L4_Bool_t L4_IsMapItem(L4_MapItem_t m) {
  return (m.raw[0] & 0xe) == 0x8;
}

static inline L4_Fpage_t L4_MapItemSndFpage(L4_MapItem_t m) {
  return (L4_Fpage_t){raw: m.raw[1]};
}

static inline L4_Word_t L4_MapItemSndBase(L4_MapItem_t m) {
  return m.raw[0];
}

/* MapItems: (C++ Bindings) ------------------------------------------------*/

#if defined(__cplusplus)

static inline L4_Bool_t L4_MapItem(L4_MapItem_t m) {
  return L4_IsMapItem (m);
}

static inline L4_Fpage_t L4_SndFpage(L4_MapItem_t m) {
  return L4_MapItemSndFpage (m);
}

static inline L4_Word_t L4_SndBase (L4_MapItem_t m) {
  return L4_MapItemSndBase (m);
}

#endif

/* Messages: ---------------------------------------------------------------*/

typedef struct {
  L4_Word_t raw[64];
} L4_Msg_t;

static inline void L4_MsgPut(L4_Msg_t*  msg,     /* message object */
                             L4_Word_t  l,       /* message label  */
                             int        u,       /* #untyped words */
                             L4_Word_t* us,      /* untyped words  */
                             int        t,       /* #typed words   */
                             void*      items) { /* typed items    */
  int i = 1;
  L4_Word_t* ts = (L4_Word_t*)items;
  msg->raw[0] = (l<<16) | ((t&0x3f)<<6) | (u&0x3f); /* message tag */
  for (u+=i; i<u; i++) {       /* copy untyped words */
    msg->raw[i] = *us++;
  }
  for (t+=u; i<t; i++) {       /* copy typed words   */
    msg->raw[i] = *ts++;
  }
}

static inline void L4_MsgGet(L4_Msg_t*  msg,     /* message object */
                             L4_Word_t* us,      /* untyped words  */
                             void*      items) { /* typed items    */
  int i = 1;
  int t = ((msg->raw[0])>>6) & 0x3f;
  int u = (msg->raw[0])      & 0x3f;
  L4_Word_t* ts = (L4_Word_t*)items;
  for (u+=i; i<u; i++) {       /* copy untyped words */
    *us++ = msg->raw[i];
  }
  for (t+=u; i<t; i++) {       /* copy typed words   */
    *ts++ = msg->raw[i];
  }
}

static inline L4_MsgTag_t L4_MsgMsgTag(L4_Msg_t* msg) {
  return (L4_MsgTag_t){raw: msg->raw[0]};
}

static inline void L4_Set_MsgMsgTag(L4_Msg_t* msg, L4_MsgTag_t tag) {
  msg->raw[0] = tag.raw;
}

static inline L4_Word_t L4_MsgLabel(L4_Msg_t* msg) {
  return (msg->raw[0]) >> 16;
}

static inline void L4_Set_MsgLabel(L4_Msg_t* msg, L4_Word_t label) {
  msg->raw[0] = (label << 16) | (msg->raw[0] & 0xffff);
}

static inline void L4_MsgLoad(L4_Msg_t* msg) {
  L4_MsgTag_t tag = L4_MsgMsgTag(msg);
//printf("MsgLoad: %x, u=%d, t=%d\n",tag,L4_UntypedWords(tag),L4_TypedWords(tag));
  L4_LoadMRs(0, 1 + L4_UntypedWords(tag) + L4_TypedWords(tag), msg->raw);
}

static inline void L4_MsgStore(L4_MsgTag_t t, L4_Msg_t* msg) {
  L4_StoreMRs(1, L4_UntypedWords(t) + L4_TypedWords(t), msg->raw + 1);
  msg->raw[0] = t.raw;
}

static inline void L4_MsgClear(L4_Msg_t* msg) {
  L4_Set_MsgMsgTag(msg, (L4_MsgTag_t){raw:0});
}

static inline void L4_MsgAppendWord(L4_Msg_t* msg, L4_Word_t w) {
  L4_MsgTag_t tag = L4_MsgMsgTag(msg);
  L4_Word_t   t   = L4_TypedWords(tag);
  L4_Word_t   u   = L4_UntypedWords(tag);
//printf("MsgAppendWord: %x (t=%d, u=%d)\n", tag, t, u);
  if (t>0) {     /* if there are typed items, then make space for w */
    L4_Word_t i = u + t;
    for (; i>u; i--) { /* could be done better using pointers ...   */
      msg->raw[i+1] = msg->raw[i];
    }
  }
  msg->raw[++u] = w;                                  /* save w     */
  msg->raw[0]   = (msg->raw[0] & ~0x3f) | (u & 0x3f); /* update tag */
}

static inline void L4_MsgAppendMapItem(L4_Msg_t* msg, L4_MapItem_t m) {
  L4_MsgTag_t tag = L4_MsgMsgTag(msg);
  L4_Word_t   t   = L4_TypedWords(tag);
  L4_Word_t   u   = L4_UntypedWords(tag);
  msg->raw[1+u+t] = m.raw[0];
  msg->raw[2+u+t] = m.raw[1];
  msg->raw[0]     = (msg->raw[0] & ~0xfc0) | (((t+2) & 0x3f) << 6);
}

/* TODO: add MsgAppendGrantItem,
             MsgAppendSimpleStringItem, and
             MsgAppendStringItem
 */

/* TODO: add Put variations ... */

/* TODO: add MsgGetGrantItem and MsgGetStringItem */

static inline L4_Word_t L4_MsgWord(L4_Msg_t* msg, L4_Word_t u) {
  return msg->raw[u+1];
}

static inline void L4_MsgGetWord(L4_Msg_t* msg, L4_Word_t u, L4_Word_t* w) {
  *w = msg->raw[u+1];
}

static inline L4_Word_t L4_MsgGetMapItem(L4_Msg_t* msg,
                                         L4_Word_t t,
                                         L4_MapItem_t* m) {
  L4_Word_t u = L4_UntypedWords((L4_MsgTag_t){raw:msg->raw[0]});
  m->raw[0] = msg->raw[u+t+1];
  m->raw[1] = msg->raw[u+t+2];
  return 2; /* two words */
}

/* Messages: (C++ bindings) ------------------------------------------------*/

#if defined(__cplusplus)

static inline void L4_Put(L4_Msg_t*  msg,        /* message object */
                          L4_Word_t  l,          /* message label  */
                          int        u,          /* #untyped words */
                          L4_Word_t* us,         /* untyped words  */
                          int        t,          /* #typed words   */
                          void*      items) {    /* typed items    */
  L4_MsgPut(msg, l, u, us, t, items);
}

static inline void L4_Get(L4_Msg_t*  msg,        /* message object */
                          L4_Word_t* us,         /* untyped words  */
                          void*      items) {    /* typed items    */
  L4_MsgGet(msg, us, items);
}

static inline L4_MsgTag_t L4_MsgTag(L4_Msg_t* msg) {
  return L4_MsgMsgTag(msg);
}

static inline void L4_Set_MsgTag(L4_Msg_t* msg, L4_MsgTag_t t) {
  L4_Set_MsgMsgTag(msg, t);
}

static inline L4_Word_t L4_Label(L4_Msg_t* msg) {
  return L4_MsgLabel(msg);
}

static inline void L4_Set_Label(L4_Msg_t* msg, L4_Word_t label) {
  L4_Set_MsgLabel(msg, label);
}

static inline void L4_Load(L4_Msg_t* msg) {
  L4_MsgLoad(msg);
}

static inline void L4_Store(L4_MsgTag_t t, L4_Msg_t* msg) {
  L4_MsgStore(t, msg);
}

static inline void L4_Clear(L4_Msg_t* msg) {
  L4_MsgClear(msg);
}

static inline void L4_Append(L4_Msg_t* msg, L4_Word_t w) {
  L4_MsgAppendWord(msg, w);
}

static inline void L4_Append(L4_Msg_t* msg, L4_MapItem_t m) {
  L4_MsgAppendMapItem(msg, m);
}

/* TODO: add bindings for other Append options and for Put */

static inline L4_Word_t L4_Get(L4_Msg_t* msg, L4_Word_t u) {
  return L4_MsgWord(msg, u);
}

static inline void L4_Get(L4_Msg_t* msg, L4_Word_t t, L4_Word_t* w) {
  L4_MsgGetWord(msg, t, w);
}

static inline L4_Word_t L4_Get(L4_Msg_t* msg, L4_Word_t t, L4_MapItem_t* m) {
  return L4_MsgGetMapItem(msg, t, m);
}

/* TODO: add overloadings for Get for GrantItem and StringItem. */

#endif

/* MsgTag Flags: -----------------------------------------------------------*/

static inline L4_MsgTag_t L4_SetReceiveBlock(L4_MsgTag_t tag) {
  tag.raw |= (1<<14);
  return tag;
}

static inline L4_MsgTag_t L4_ClearReceiveBlock(L4_MsgTag_t tag) {
  tag.raw &= ~(1<<14);
  return tag;
}

static inline L4_MsgTag_t L4_SetSendBlock(L4_MsgTag_t tag) {
  tag.raw |= (1<<15);
  return tag;
}

static inline L4_MsgTag_t L4_ClearSendBlock(L4_MsgTag_t tag) {
  tag.raw &= ~(1<<15);
  return tag;
}

/* IPC System Call: --------------------------------------------------------*/

EXTERNC(L4_Word_t L4_Prim_Ipc
                        (L4_ThreadId_t to,
                         L4_ThreadId_t fromSpec,
                         L4_ThreadId_t* from))

static inline L4_MsgTag_t L4_Ipc(L4_ThreadId_t to,
                                 L4_ThreadId_t fromSpec,
                                 L4_ThreadId_t* from) {
  return (L4_MsgTag_t){raw:L4_Prim_Ipc(to, fromSpec, from)};
}

/* TODO: implement a real Lipc ... */
static inline L4_MsgTag_t L4_Lipc(L4_ThreadId_t to,
                                  L4_ThreadId_t fromSpec,
                                  L4_ThreadId_t* from) {
  return (L4_MsgTag_t){raw:L4_Prim_Ipc(to, fromSpec, from)};
}

/* --- Specializations of IPC --- */

/* call: send and receive back from same thread, blocking allowed */
static inline L4_MsgTag_t L4_Call(L4_ThreadId_t to) {
  L4_Set_MsgTag(L4_SetReceiveBlock(L4_SetSendBlock(L4_MsgTag())));
  L4_ThreadId_t from;
  return L4_Ipc(to, to, &from);
}

/* send: send only, blocking allowed */
static inline L4_MsgTag_t L4_Send(L4_ThreadId_t to) {
  L4_Set_MsgTag(L4_SetSendBlock(L4_MsgTag()));
  L4_ThreadId_t from;
//printf("Send: (tag %x) %x, %x\n", L4_MsgTag(), to, L4_nilthread);
  return L4_Ipc(to, L4_nilthread, &from);
}

/* reply: send only to waiting thread, NO blocking allowed */
static inline L4_MsgTag_t L4_Reply(L4_ThreadId_t to) {
  L4_Set_MsgTag(L4_ClearSendBlock(L4_MsgTag()));
  L4_ThreadId_t from;
  return L4_Ipc(to, L4_nilthread, &from);
}

/* receive: receive only, blocking allowed */
static inline L4_MsgTag_t L4_Receive(L4_ThreadId_t fromSpec) {
  L4_Set_MsgTag(L4_SetReceiveBlock(L4_MsgTag()));
  L4_ThreadId_t from;
  return L4_Ipc(L4_nilthread, fromSpec, &from);
}

/* wait: wait from any, blocking allowed */
static inline L4_MsgTag_t L4_Wait(L4_ThreadId_t* from) {
  L4_Set_MsgTag(L4_SetReceiveBlock(L4_MsgTag()));
//  printf("Wait: (tag %x) %x, %x\n", L4_MsgTag(), L4_nilthread, L4_anythread);
  return L4_Ipc(L4_nilthread, L4_anythread, from);
}

/* replywait: server send reply and then wait for next request */
static inline L4_MsgTag_t L4_ReplyWait(L4_ThreadId_t to, L4_ThreadId_t* from) {
  L4_Set_MsgTag(L4_SetReceiveBlock(L4_ClearSendBlock(L4_MsgTag())));
  return L4_Ipc(to, L4_anythread, from);
}

/* lcall: local send and receive back from same thread, blocking allowed */
static inline L4_MsgTag_t L4_Lcall(L4_ThreadId_t to) {
  L4_Set_MsgTag(L4_SetReceiveBlock(L4_SetSendBlock(L4_MsgTag())));
  L4_ThreadId_t from;
  return L4_Lipc(to, to, &from);
}

/* SUPPORT FUNCTIONS */

static inline L4_Bool_t L4_IpcFailed(L4_MsgTag_t t) {
  return (t.raw & (1<<15));
}

static inline L4_Bool_t L4_IpcSucceeded(L4_MsgTag_t t) {
  return L4_IpcFailed(t)==0;
}

#endif
