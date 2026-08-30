#include <SDL3/SDL.h>
#include <stdio.h>
int main(void){int b=-1;SDL_MessageBoxButtonData buttons[]={{0,10,"Yes"},{0,20,"No"},{SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT,30,"OK"}};
SDL_MessageBoxData d={SDL_MESSAGEBOX_INFORMATION,NULL,"SDL3 AmigaOS3","MessageBox via EasyRequest",SDL_arraysize(buttons),buttons,NULL};
if(!SDL_ShowMessageBox(&d,&b)){printf("failed: %s\n",SDL_GetError());return 1;}printf("button id=%d\n",b);return 0;}