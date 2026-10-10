#include "box_ui.h"
int lbx_grid_hit(int x,int y,int first,int count) {
    if(x<10 || x>=310 || y<48 || y>=198) return -1;
    int col=(x-10)/50,row=(y-48)/30;
    if((x-10)%50>=48 || (y-48)%30>=28) return -1;
    int index=first+row*6+col;
    return index<count?index:-1;
}
int lbx_grid_move(int cursor,int count,int dx,int dy) {
    if(count<=0) return 0;
    int next=cursor+dx+dy*6;
    if(next<0) return 0;
    if(next>=count) return count-1;
    return next;
}
