#include "frontend.h"
#include <assert.h>
#include <stdio.h>
int main(int argc,char** argv) {
    (void)argc; LbxFrontend s;char error[160];lbx_frontend_init(&s);
    assert(!lbx_save_request(&s,argv[1],error,sizeof(error)));
    lbx_preset(&s,0);assert(lbx_selected_count(&s)==9);
    assert(lbx_save_request(&s,argv[1],error,sizeof(error)));
    lbx_preset(&s,1);assert(lbx_selected_count(&s)==807);
    s.start_box=31;assert(!lbx_save_request(&s,argv[1],error,sizeof(error)));
    lbx_preset(&s,2);assert(s.shiny==1 && lbx_selected_count(&s)==9);
    lbx_preset(&s,3);assert(lbx_selected_count(&s)==0);
    puts("Frontend core PASS");return 0;
}
