#include <assert.h>
#include "common.h"
#include "point.h"
#include <math.h>

void
point_translate(struct point *p, double x, double y)
{
    double oldX=p->x;
    double oldY=p->y;
    p->x=oldX+x;
    p->y=oldY+y;
}

double
point_distance(const struct point *p1, const struct point *p2)
{
    double x1=p1->x;
    double x2=p2->x;
    double y1=p1->y;
    double y2=p2->y;
    double distance=0;
    distance=sqrt(pow(x2-x1,2.0)+pow(y2-y1,2.0));
	return distance;
}

int
point_compare(const struct point *p1, const struct point *p2)
{
    double x1=p1->x;
    double x2=p2->x;
    double y1=p1->y;
    double y2=p2->y;
    double distance1=0,distance2=0;
    
    distance1=sqrt(pow(x1,2.0)+pow(y1,2.0));
    distance2=sqrt(pow(x2,2.0)+pow(y2,2.0));
    
    if(distance1>distance2){
        return 1;
    }
    else if(distance1<distance2){
        return -1;
    }else{
        return 0;
    }	
}
