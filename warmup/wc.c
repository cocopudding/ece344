#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "common.h"
#include "wc.h"
#include <limits.h>
#include <string.h>


struct wc {
	/* you can define this struct to have whatever fields you want. */
    char word[100];
    int key;
    struct wc *link;
    int size;
    int entries;
    int empty;
    int checked;
    
};

//hash function found online from https://gist.github.com/tonious/1377667
int hash(char *str,int size)
    {
       unsigned long int hashval;
	int i = 0;

	//Convert our string to an integer 
	while( hashval < ULONG_MAX && i < strlen( str ) ) {
		hashval = hashval << 8;
		hashval += str[ i ];
		i++;
	}
        
       /* if(hashval<0){
            hashval=0-hashval;
        }*/
        
      // printf("!string: %s  hashvalue: %d\n",str,hashval% size);
        return hashval% size;
   
    /*int i, sum;
  for (sum=0, i=0; i < table->size; i++)
    sum += str[i];
  return sum % table->size;*/
    }


 
 
struct wc *
wc_init(char *word_array, long size)   
{  
    long size2=size;
   if(size>100000000){
       size2=100000000;
    }
  

	struct wc *wc;
        
       //printf("ok1\n");
	wc = (struct wc *)malloc(sizeof(struct wc)*size2);
	assert(wc);
        
     // printf("ok2\n");
        wc->size=size2;

        
      //printf("ok3\n");
        int i=0;
        int tempi=0;
        char tempString[100];
        int checkPrev=0;
        
        for(i=0;i<100;i++){
            tempString[i]='\0';
        }
        i=0;
        while (i<size2){
            wc[i].link=NULL;
            i++;
        }
        i=0;
        
        for (i=0;i<size2;i++){
            wc[i].empty=0;
        }
        i=0;
        
     // printf("ok4\n");
        while(i<size2){
           //printf("original:%c\n",word_array[i]);
            if(isspace(word_array[i])==0&&word_array[i]!='\0'&&word_array[i]!='\n'){
                tempString[tempi]=word_array[i];
             //printf("tempi:%d\n",tempi);
              // printf("newstring:%s\n",tempString);
                i++;
                tempi++;
                checkPrev=1;
            }
            else{
                if((isspace(word_array[i])!=0||word_array[i]=='\0')&&(checkPrev==1||i==0)){
                    checkPrev=0;
                    
                    int x=tempi;
                    while(x<100){
                        tempString[x]='\0';
                        x++;
                    }
                    char *newtempString=(char *)malloc(sizeof(char)*(tempi+1));
                    strncpy(newtempString,tempString,tempi+1);
                  //printf("*************newstring:%s************\n",newtempString);
                int hashnum=hash(newtempString,size2);
                if(wc[hashnum].word[0]=='\0'||wc[hashnum].empty==0){
                     strncpy ( wc[hashnum].word, newtempString, sizeof(wc[hashnum].word) );
                     wc[hashnum].link=NULL;
                     wc[hashnum].empty=1;       
                     wc[hashnum].entries=1;
                }
                else{
                    struct wc *newNode=(struct wc *)malloc(sizeof(struct wc));
                    struct wc *temp;
                    newNode->entries=1;
                    strncpy (newNode->word, newtempString,sizeof(newNode->word));
                    newNode->link=NULL;
                   // printf("same hash!~~~~!!!!!!!!\n");
                 //  printf("newstring:%s************\n",newNode->word);
                    
                    if(wc[hashnum].link==NULL){
                        if(strcmp(newtempString,wc[hashnum].word)==0){
                            wc[hashnum].entries++;
                            free(newNode);
                        }
                        else{
                            wc[hashnum].link=newNode;
                        }  
                       //printf("ok1\n");
                    }
                    else{
                        
                        //printf("ok2\n");
                        
                        
                        if(strcmp(newtempString,wc[hashnum].word)==0){
                            wc[hashnum].entries++;
                            free(newNode);
                        }
                        else{
                            temp=wc[hashnum].link;
                            
                            while(temp->link!=NULL){
                                if(strcmp(newtempString,temp->word)==0)
                                {
                                    temp->entries++;
                                    break;
                                }
                            temp=temp->link;
                            }
                             if(strcmp(newtempString,temp->word)==0)
                             {
                                    temp->entries++;
                                    free(newNode);
                             }
                             else{
                                  temp->link=newNode;
                             }
                           
                        }
                    }                     
                }
                int w=0;
                for(w=0;w<100;w++){
                 tempString[w]='\0';
                }
                free(newtempString);
             // printf("entries:%d\n",wc->entries);
                i++;
                tempi=0;
                }
                else{
                    //printf("!!!extra space\n");
                    i++;
                    tempi=0;
                }
            }
        }

	return wc;
}

void
wc_output(struct wc *wc)
{
    //printf("!!!!!!!!!!!size:%d\n",wc->size);
    int i=0;    
    struct wc *temp;
    
    
    while(i<wc->size){
        if(wc[i].empty!=0){
            printf("%s:%d\n",wc[i].word,wc[i].entries);
            if(wc[i].link!=NULL){
                temp=wc[i].link;
                while(temp!=NULL){
                    printf("%s:%d\n",temp->word,temp->entries);
                    temp=temp->link;
                }
            }
        }
        i++;
    }
    
   /* struct wc *temp2=&wc[313];
    while(temp2!=NULL){
        printf("**%s:%d\n",temp2->word,temp2->entries);
        temp2=temp2->link;
    }*/
   
}

void
wc_destroy(struct wc *wc)
{
    struct wc * temp;
        int i = 0;
        for(i=0;i<wc->size;i++)
        {
            if (wc[i].empty!=0)
            {  
                if(wc[i].link != NULL){
                    //wc[i].link = wc[i].link->link; 
                    temp=wc[i].link;
                    while(temp != NULL)
                    {
                        struct wc * temp2 = temp;
                        temp = temp->link;
                        free(temp2);
                    }
                }
            }       
        }
    free(wc);
}


/*

int main (){
 
    char text[]="hello hello you are ugly bye bye see you tm,r dslkfjsal dslkf asdn sd a hello\n !";
    long length=(long)sizeof (text)*sizeof(char);
    //printf("length %lu\n",length);
    struct wc *wc=wc_init(text,length );
   wc_output(wc);
   wc_destroy(wc);
    
    
    return 0;
}
*/