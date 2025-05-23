/*
 * (c) 2008-2009 Jork Löser <jork@os.inf.tu-dresden.de>
 *     economic rights: Technische Universität Dresden (Germany)
 * License: see LICENSE.spdx (in this directory or the directories above)
 */
#include <l4/util/spin.h>

static void spin_gen(void*addr,int x,int y){
  unsigned char c,*p;
    
  p=(unsigned char*)addr+(x+80*y)*2;
  c=*p;
  c=(c=='|')?'/':(c=='/')?'-':(c=='-')?'\\':(c=='\\')?'|':'-';
  *p=c;
}

/****************************************************************************
*                                                                           *
*  l4_spin()     - spinning wheel at the hercules screen, position is from  *
*                  upper left. Each call turns the wheel.                   *
*  l4_spin_vga() - the same for vga.                                        *
*                                                                           *
****************************************************************************/
L4_CV void l4_spin(int x,int y){
  spin_gen((void*)0xb0000, x, y);
}
L4_CV void l4_spin_vga(int x, int y){
  spin_gen((void*)0xb8000, x, y);
}

static void spin_n_text_gen(void*addr, int x,int y, int len, const char*s){
  unsigned char c,*p;
  int i;
  
  p=(unsigned char*)addr+(x+len+80*y)*2;
  c=*p;
  c=(c=='|')?'/':(c=='/')?'-':(c=='-')?'\\':(c=='\\')?'|':'.';
  if(c=='.'){
    if(s){
      p=(unsigned char*)addr+(x+80*y)*2;
      for(i=0;i<len;i++){
        *p++ = *s++;p++;
      }
    }
    c = '-';
  }
  *p=c;
}

/****************************************************************************
*                                                                           *
*  l4_spin_n_text()     - like spin(), but prints a text before the wheel.  *
*                         You must specify the length of the text (without  *
*                         the 0 at the end). The text is printed if no      *
*                         wheel-element is found at the wheel position.     *
*                         See macro l4_spin_text() for constant text.       *
*  l4_spin_n_text_vga() - same for vga.                                     *
*                                                                           *
****************************************************************************/
L4_CV void l4_spin_n_text(int x,int y, int len, const char*s){
  spin_n_text_gen((void*)0xb0000, x, y, len, s);
}
L4_CV void l4_spin_n_text_vga(int x,int y, int len, const char*s){
  spin_n_text_gen((void*)0xb8000, x, y, len, s);
}
