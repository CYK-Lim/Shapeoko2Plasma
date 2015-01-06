/*
 to compile, go to linuxcnc-rtos/src/hal/components
 
 run sudo comp --install tblcompkins.c
*/

#include "kinematics.h"

#ifdef RTAPI

#include "rtapi.h"      /* RTAPI realtime OS API */
#include "rtapi_app.h"      /* RTAPI realtime module decls */
#include "hal.h"

struct haldata {
    hal_float_t *xslope;
    hal_float_t *yslope;
} *haldata;

int kinematicsForward(const double *joints,
              EmcPose * pos,
              const KINEMATICS_FORWARD_FLAGS * fflags,
              KINEMATICS_INVERSE_FLAGS * iflags)
{
    pos->tran.x = joints[0];// + joints[1]*(*(haldata->xslope));
    pos->tran.y = joints[1];
    pos->tran.z = joints[2] - joints[0]*(*(haldata->xslope)) - joints[1]*(*(haldata->yslope));
    pos->a = joints[3];
    pos->b = joints[4];
    pos->c = joints[5];
    pos->u = joints[6];
    pos->v = joints[7];
    pos->w = joints[8];

    return 0;
}

int kinematicsInverse(const EmcPose * pos,
              double *joints,
              const KINEMATICS_INVERSE_FLAGS * iflags,
              KINEMATICS_FORWARD_FLAGS * fflags)
{
    joints[0] = pos->tran.x;// - pos->tran.y*(*(haldata->xslope));
    joints[1] = pos->tran.y;
    joints[2] = pos->tran.z + pos->tran.x*(*(haldata->xslope)) + pos->tran.y*(*(haldata->yslope));
    joints[3] = pos->a;
    joints[4] = pos->b;
    joints[5] = pos->c;
    joints[6] = pos->u;
    joints[7] = pos->v;
    joints[8] = pos->w;

    return 0;
}

/* implemented for these kinematics as giving joints preference */
int kinematicsHome(EmcPose * world,
           double *joint,
           KINEMATICS_FORWARD_FLAGS * fflags,
           KINEMATICS_INVERSE_FLAGS * iflags)
{
    *fflags = 0;
    *iflags = 0;

    return kinematicsForward(joint, world, fflags, iflags);
}

KINEMATICS_TYPE kinematicsType()
{
    return KINEMATICS_IDENTITY;
}

EXPORT_SYMBOL(kinematicsType);
EXPORT_SYMBOL(kinematicsForward);
EXPORT_SYMBOL(kinematicsInverse);
MODULE_LICENSE("GPL");

int comp_id;
int rtapi_app_main(void) {
    int res = 0;
    comp_id = hal_init("tblcompkins");
    if(comp_id < 0) return comp_id;

    do {
      haldata = hal_malloc(sizeof(struct haldata));
      if(!haldata) break;

      res = hal_pin_float_new("tblcompkins.xslope", HAL_IN, &(haldata->xslope), comp_id);
      if (res < 0) break;
      
      res = hal_pin_float_new("tblcompkins.yslope", HAL_IN, &(haldata->yslope), comp_id);
      if (res < 0) break;

      hal_ready(comp_id);
      return 0;
    } while (0);

    hal_exit(comp_id);
    return comp_id;
}

void rtapi_app_exit(void)
{
  hal_exit(comp_id);
}
#endif
