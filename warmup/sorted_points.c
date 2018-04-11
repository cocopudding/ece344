#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "point.h"
#include "sorted_points.h"
#include <math.h>


struct sorted_points {
	/* you can define this struct to have whatever fields you want. */
    double x;
    double y;
    double d;
    struct sorted_points *link;
};

struct sorted_points *
sp_init()
{
	struct sorted_points *sp;

	sp = (struct sorted_points *)malloc(sizeof(struct sorted_points));
	assert(sp);
        
        sp->x=0.0;
        sp->y=0.0;
        sp->d=0.0;
        sp->link=NULL;

	return sp;
}

void
sp_destroy(struct sorted_points *sp)
{       
        struct sorted_points *temp;
        while(sp!=NULL){
            temp=sp;
            sp=sp->link;
            free(temp);
        }
}

int
sp_add_point(struct sorted_points *sp, double x, double y)
{
	struct sorted_points *current=sp;
        struct sorted_points *previous=NULL;
        
        int located=0;
        while(located==0&&current!=NULL){
            if(sqrt(pow(x,2.0)+pow(y,2.0))<(current->d)){      
                located=1;
            }
            else if(sqrt(pow(x,2.0)+pow(y,2.0))==current->d&&x<current->x){
                located=1;
            }
            else{
                previous=current;
                current=current->link;
            }
        }
        
        struct sorted_points *newNode=(struct sorted_points *)malloc(sizeof(struct sorted_points));
        if(newNode==NULL){
            return 0;
        }
        else{
            newNode->x=x;
            newNode->y=y;
            newNode->d=sqrt(pow(x,2.0)+pow(y,2.0));
            newNode->link=NULL;
            if(current==sp){
                sp=newNode;
            }
            else{
                previous->link=newNode;
                newNode->link=current;
            }                
            return 1;
        }
}

int
sp_remove_first(struct sorted_points *sp, struct point *ret)
{       
        if(sp->link==NULL){
            return 0;
        }
        else{
            struct sorted_points *current=sp->link;
            sp->link=current->link;
            ret->x=current->x;
            ret->y=current->y;
            free(current);
            return 1;
        }        
}

int
sp_remove_last(struct sorted_points *sp, struct point *ret)
{
	if(sp->link==NULL){
            return 0;
        }
        else{
            struct sorted_points *prev=sp;
            struct sorted_points *current=sp->link;
            while(current->link!=NULL){
                current=current->link;
                prev=prev->link;
            }
            ret->x=current->x;
            ret->y=current->y;
            free(current);
            prev->link=NULL;
            return 1;
            
        }
}

int
sp_remove_by_index(struct sorted_points *sp, int index, struct point *ret)
{
	if(sp->link==NULL){
            return 0;
        }
        else{
            struct sorted_points *prev=sp;            
            struct sorted_points *current=sp->link;
            int n=0;
            while(current->link!=NULL&&n<index){
                current=current->link;
                prev=prev->link;
                n++;
            }
            if(n!=index){
                return 0;
            }
            else{
                prev->link=current->link;
                ret->x=current->x;
                ret->y=current->y;
                free(current);
                return 1;
            }
        }
        
}

int
sp_delete_duplicates(struct sorted_points *sp)
{
        int deleted=0;
        
	if(sp->link==NULL||sp->link->link==NULL){
            return 0;
        }
        else{
            struct sorted_points *prev=sp->link;            
            struct sorted_points *current=sp->link->link;
            while(prev->link!=NULL){
                if((current->x==prev->x)&&(current->y==prev->y)){
                    prev->link=current->link;
                    free(current);
                    deleted++;
                    current=prev->link;
                }
                else{
                    current=current->link;
                    prev=prev->link;
                }
            }
            return deleted;
        }
}


