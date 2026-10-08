#include "templates.h"
#include "legacybox.h"
#include <assert.h>
#include <stdio.h>
int main(int argc,char** argv) {
    (void)argv;
    LbxFrontend s;char error[160];lbx_frontend_init(&s);
    s.selected[0]=true;s.selected[1]=true;s.start_box=7;
    if(argc>1) {assert(!lbx_assemble_templates(&s,error,sizeof(error)));puts("Tampered template rejected");return 0;}
    assert(lbx_assemble_templates(&s,error,sizeof(error)));
    LbxBatch b;int count=0;
    assert(lbx_load_batch(&b,error,sizeof(error)));
    assert(b.count==2 && b.start_box==7);
    assert(lbx_verify_batch(&b,&count,error,sizeof(error)) && count==2);
    assert(!lbx_assemble_templates(&s,error,sizeof(error)));
    puts("Offline assembly PASS");return 0;
}
