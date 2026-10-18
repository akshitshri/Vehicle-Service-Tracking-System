/* Vehicle Service Tracking System - lightweight C HTTP server
 * Data structures: singly linked list for records, linked FIFO queue.
 */
#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
typedef SOCKET socket_t;
#define CLOSESOCKET closesocket
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <strings.h>
typedef int socket_t;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#define CLOSESOCKET close
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "service.h"

#define PORT 8080
#define REQ_SIZE 65536
#define BODY_SIZE 32768

typedef struct Record {
    int id;
    char customer[NAME_LEN], phone[32], vehicle[VEHICLE_LEN];
    char vehicleType[32], serviceType[TYPE_LEN], date[16], priority[16];
    char issue[512], status[STATUS_LEN], createdAt[40];
    struct Record *next;
} Record;
typedef struct QNode { int id; struct QNode *next; } QNode;
typedef struct DiskRecord {
    int id;
    char customer[NAME_LEN], phone[32], vehicle[VEHICLE_LEN];
    char vehicleType[32], serviceType[TYPE_LEN], date[16], priority[16];
    char issue[512], status[STATUS_LEN], createdAt[40];
} DiskRecord;
static Record *records = NULL, *tail = NULL;
static QNode *qfront = NULL, *qrear = NULL;
static int nextId = 1001;

static void clean_field(char *s) {
    for (; *s; ++s) if (*s == '\n' || *s == '\r' || *s == '\t') *s = ' ';
}
static void copy_field(char *dst, size_t cap, const char *src) {
    if (!cap) return;
    snprintf(dst, cap, "%s", src ? src : ""); clean_field(dst);
}
static Record *find_record(int id) {
    for (Record *r = records; r; r = r->next) if (r->id == id) return r;
    return NULL;
}
static int queued(int id) { for (QNode *q=qfront; q; q=q->next) if(q->id==id) return 1; return 0; }
static void persist(void) {
    FILE *f = fopen("backend/services.dat", "wb");
    if (f) { for (Record *r=records; r; r=r->next) { DiskRecord d; memset(&d,0,sizeof d); d.id=r->id;
        memcpy(d.customer,r->customer,sizeof d.customer); memcpy(d.phone,r->phone,sizeof d.phone); memcpy(d.vehicle,r->vehicle,sizeof d.vehicle);
        memcpy(d.vehicleType,r->vehicleType,sizeof d.vehicleType); memcpy(d.serviceType,r->serviceType,sizeof d.serviceType); memcpy(d.date,r->date,sizeof d.date);
        memcpy(d.priority,r->priority,sizeof d.priority); memcpy(d.issue,r->issue,sizeof d.issue); memcpy(d.status,r->status,sizeof d.status); memcpy(d.createdAt,r->createdAt,sizeof d.createdAt);
        fwrite(&d,sizeof d,1,f); } fclose(f); }
    f = fopen("backend/queue.dat", "wb"); if (f) { for(QNode*q=qfront;q;q=q->next) fwrite(&q->id,sizeof(int),1,f); fclose(f); }
}
static void load_data(void) {
    FILE *f=fopen("backend/services.dat","rb"); if(f){ DiskRecord d; while(fread(&d,sizeof d,1,f)==1){ Record*r=calloc(1,sizeof *r); if(!r)break; r->id=d.id;
        memcpy(r->customer,d.customer,sizeof r->customer); memcpy(r->phone,d.phone,sizeof r->phone); memcpy(r->vehicle,d.vehicle,sizeof r->vehicle);
        memcpy(r->vehicleType,d.vehicleType,sizeof r->vehicleType); memcpy(r->serviceType,d.serviceType,sizeof r->serviceType); memcpy(r->date,d.date,sizeof r->date);
        memcpy(r->priority,d.priority,sizeof r->priority); memcpy(r->issue,d.issue,sizeof r->issue); memcpy(r->status,d.status,sizeof r->status); memcpy(r->createdAt,d.createdAt,sizeof r->createdAt);
        if(tail)tail->next=r;else records=r;tail=r;if(r->id>=nextId)nextId=r->id+1; } fclose(f); }
    f=fopen("backend/queue.dat","rb"); if(f){int id;while(fread(&id,sizeof id,1,f)==1)if(find_record(id)&&!queued(id)){QNode*q=calloc(1,sizeof*q);if(!q)break;q->id=id;if(qrear)qrear->next=q;else qfront=q;qrear=q;}fclose(f);}
}
static void json_escape(const char *s, char *out, size_t cap) {
    size_t j=0; if(!cap)return; for(size_t i=0;s && s[i] && j+2<cap;i++){unsigned char c=(unsigned char)s[i];
        if(c=='"'||c=='\\'){out[j++]='\\';out[j++]=(char)c;} else if(c=='\n'){out[j++]='\\';out[j++]='n';} else if(c=='\r'){out[j++]='\\';out[j++]='r';} else if(c=='\t'){out[j++]='\\';out[j++]='t';} else if(c>=32)out[j++]=(char)c; }
    out[j]='\0';
}
static void json_string(const char *body,const char *key,char*out,size_t cap){
    char pat[80];snprintf(pat,sizeof pat,"\"%s\"",key);const char*p=strstr(body,pat);if(!p){if(cap)out[0]=0;return;}p=strchr(p+strlen(pat),':');if(!p){if(cap)out[0]=0;return;}p++;while(*p&&isspace((unsigned char)*p))p++;if(*p!='\"'){if(cap)out[0]=0;return;}p++;size_t j=0;
    while(*p&&*p!='\"'&&j+1<cap){
        if(*p=='\\'&&p[1]){p++;char c=*p++;if(c=='n'||c=='r'||c=='t')out[j++]=' ';else out[j++]=c;}
        else out[j++]=*p++;
    }
    out[j]=0;clean_field(out);
}
static int json_int(const char*body,const char*key,int fallback){char pat[80];snprintf(pat,sizeof pat,"\"%s\"",key);const char*p=strstr(body,pat);if(!p)return fallback;p=strchr(p+strlen(pat),':');if(!p)return fallback;return atoi(p+1);}
static void append_record_json(char *out,size_t cap,size_t *used,Record*r){char a[2048];char c[NAME_LEN*2],p[64],v[VEHICLE_LEN*2],vt[64],st[TYPE_LEN*2],dt[32],pr[32],is[1024],ss[80],ca[80];
    json_escape(r->customer,c,sizeof c);json_escape(r->phone,p,sizeof p);json_escape(r->vehicle,v,sizeof v);json_escape(r->vehicleType,vt,sizeof vt);json_escape(r->serviceType,st,sizeof st);json_escape(r->date,dt,sizeof dt);json_escape(r->priority,pr,sizeof pr);json_escape(r->issue,is,sizeof is);json_escape(r->status,ss,sizeof ss);json_escape(r->createdAt,ca,sizeof ca);
    snprintf(a,sizeof a,"%s{\"id\":%d,\"customer\":\"%s\",\"phone\":\"%s\",\"vehicle\":\"%s\",\"vehicleType\":\"%s\",\"serviceType\":\"%s\",\"date\":\"%s\",\"priority\":\"%s\",\"issue\":\"%s\",\"status\":\"%s\",\"createdAt\":\"%s\"}",*used>1?",":"",r->id,c,p,v,vt,st,dt,pr,is,ss,ca);
    size_t n=strlen(a);if(*used+n+2<cap){memcpy(out+*used,a,n);*used+=n;out[*used]=0;}
}
static void send_response(socket_t client,int code,const char*type,const char*body){char hdr[512];const char*reason=code==200?"OK":code==201?"Created":code==400?"Bad Request":code==404?"Not Found":code==500?"Internal Server Error":"OK";size_t len=strlen(body);
    snprintf(hdr,sizeof hdr,"HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %lu\r\nConnection: close\r\nCache-Control: no-store\r\nAccess-Control-Allow-Origin: *\r\nAccess-Control-Allow-Headers: Content-Type\r\nAccess-Control-Allow-Methods: GET, POST, PATCH, DELETE, OPTIONS\r\n\r\n",code,reason,type,(unsigned long)len);
    send(client,hdr,(int)strlen(hdr),0);send(client,body,(int)len,0);
}
static void send_json(socket_t c,int code,const char*body){send_response(c,code,"application/json; charset=utf-8",body);}
static void get_services(socket_t c){char*out=calloc(1,60000);if(!out){send_json(c,500,"{\"error\":\"Out of memory\"}");return;}strcpy(out,"[");size_t used=1;for(Record*r=records;r;r=r->next)append_record_json(out,60000,&used,r);if(used+2<60000){out[used++]=']';out[used]=0;}send_json(c,200,out);free(out);}
static void get_queue(socket_t c){char out[12000]="[";size_t used=1;for(QNode*q=qfront;q;q=q->next){char part[40];snprintf(part,sizeof part,"%s%d",used>1?",":"",q->id);size_t n=strlen(part);if(used+n+2<sizeof out){memcpy(out+used,part,n);used+=n;out[used]=0;}}out[used++]=']';out[used]=0;send_json(c,200,out);}
static void create_record(socket_t c,const char*body){Record*r=calloc(1,sizeof*r);if(!r){send_json(c,500,"{\"error\":\"Out of memory\"}");return;}r->id=nextId++;
    json_string(body,"customer",r->customer,sizeof r->customer);json_string(body,"phone",r->phone,sizeof r->phone);json_string(body,"vehicle",r->vehicle,sizeof r->vehicle);json_string(body,"vehicleType",r->vehicleType,sizeof r->vehicleType);json_string(body,"serviceType",r->serviceType,sizeof r->serviceType);json_string(body,"date",r->date,sizeof r->date);json_string(body,"priority",r->priority,sizeof r->priority);json_string(body,"issue",r->issue,sizeof r->issue);
    if(!r->customer[0]||!r->vehicle[0]||!r->serviceType[0]){free(r);send_json(c,400,"{\"error\":\"Customer, vehicle and service type are required\"}");return;}
    if(!r->vehicleType[0])strcpy(r->vehicleType,"Car");if(!r->priority[0])strcpy(r->priority,"Normal");strcpy(r->status,"Booked");time_t now=time(NULL);struct tm *tmv=localtime(&now);if(tmv)strftime(r->createdAt,sizeof r->createdAt,"%Y-%m-%dT%H:%M:%S",tmv);
    if(tail)tail->next=r;else records=r;tail=r;persist();char out[4096];size_t used=0;out[0]=0;append_record_json(out,sizeof out,&used,r);send_json(c,201,out);
}
static void add_queue(socket_t c,int id){Record*r=find_record(id);if(!r){send_json(c,404,"{\"error\":\"Service record not found\"}");return;}if(queued(id)){send_json(c,200,"{\"ok\":true,\"message\":\"Already queued\"}");return;}if(!strcmp(r->status,"Completed")||!strcmp(r->status,"Cancelled")){send_json(c,400,"{\"error\":\"Closed service cannot be queued\"}");return;}QNode*q=calloc(1,sizeof*q);if(!q){send_json(c,500,"{\"error\":\"Out of memory\"}");return;}q->id=id;if(qrear)qrear->next=q;else qfront=q;qrear=q;persist();send_json(c,201,"{\"ok\":true}");}
static void remove_queue_id(int id){QNode*prev=NULL,*q=qfront;while(q){if(q->id==id){QNode*n=q->next;if(prev)prev->next=n;else qfront=n;if(qrear==q)qrear=prev;free(q);q=n;}else{prev=q;q=q->next;}}}
static void delete_record(socket_t c,int id){Record*prev=NULL,*r=records;while(r&&r->id!=id){prev=r;r=r->next;}if(!r){send_json(c,404,"{\"error\":\"Service record not found\"}");return;}if(prev)prev->next=r->next;else records=r->next;if(tail==r)tail=prev;free(r);remove_queue_id(id);persist();send_json(c,200,"{\"ok\":true}");}
static void update_status(socket_t c,int id,const char*body){Record*r=find_record(id);if(!r){send_json(c,404,"{\"error\":\"Service record not found\"}");return;}char status[STATUS_LEN];json_string(body,"status",status,sizeof status);const char*valid[]={"Booked","Received","Inspection","In Progress","Ready for Pickup","Completed","Cancelled"};int ok=0;for(size_t i=0;i<sizeof(valid)/sizeof(valid[0]);i++)if(!strcmp(status,valid[i]))ok=1;if(!ok){send_json(c,400,"{\"error\":\"Invalid status\"}");return;}strcpy(r->status,status);if(!strcmp(status,"In Progress")||!strcmp(status,"Completed")||!strcmp(status,"Cancelled"))remove_queue_id(id);persist();char out[4096];size_t used=0;out[0]=0;append_record_json(out,sizeof out,&used,r);send_json(c,200,out);}
static void process_queue(socket_t c){while(qfront){QNode*q=qfront;qfront=q->next;if(!qfront)qrear=NULL;int id=q->id;free(q);Record*r=find_record(id);if(!r||!strcmp(r->status,"Completed")||!strcmp(r->status,"Cancelled"))continue;
    if(!strcmp(r->status,"Booked"))strcpy(r->status,"Received");else if(!strcmp(r->status,"Received"))strcpy(r->status,"Inspection");else strcpy(r->status,"In Progress");if(strcmp(r->status,"In Progress")){QNode*again=calloc(1,sizeof *again);if(again){again->id=id;if(qrear)qrear->next=again;else qfront=again;qrear=again;}}char out[4096];size_t used=0;out[0]=0;append_record_json(out,sizeof out,&used,r);persist();send_json(c,200,out);return;}persist();send_json(c,404,"{\"error\":\"Queue is empty\"}");}
static void serve_static(socket_t c,const char*path){char file[512];const char*rel=path;if(!strcmp(path,"/"))rel="/index.html";if(strstr(rel,"..")){send_response(c,400,"text/plain","Bad path");return;}snprintf(file,sizeof file,"frontend%s",rel);FILE*f=fopen(file,"rb");if(!f){send_response(c,404,"text/plain; charset=utf-8","File not found. Start the server from the project root folder.");return;}fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);if(n<0||n>2000000){fclose(f);send_response(c,500,"text/plain","File too large");return;}char*data=malloc((size_t)n+1);if(!data){fclose(f);send_response(c,500,"text/plain","Out of memory");return;}fread(data,1,(size_t)n,f);fclose(f);data[n]=0;const char*type="text/plain; charset=utf-8";if(strstr(file,".html"))type="text/html; charset=utf-8";else if(strstr(file,".css"))type="text/css; charset=utf-8";else if(strstr(file,".js"))type="application/javascript; charset=utf-8";send_response(c,200,type,data);free(data);}
static int handle_request(socket_t c,char*req){char method[12]="",path[256]="";sscanf(req,"%11s %255s",method,path);char*body=strstr(req,"\r\n\r\n");body=body?body+4:"";
    if(!strcmp(method,"OPTIONS")){send_json(c,200,"{}");return 0;}
    if(!strncmp(path,"/api/",5)){
        if(!strcmp(method,"GET")&&!strcmp(path,"/api/health")){send_json(c,200,"{\"ok\":true,\"backend\":\"C\"}");return 0;}
        if(!strcmp(method,"GET")&&!strcmp(path,"/api/services")){get_services(c);return 0;}
        if(!strcmp(method,"GET")&&!strcmp(path,"/api/queue")){get_queue(c);return 0;}
        if(!strcmp(method,"POST")&&!strcmp(path,"/api/services")){create_record(c,body);return 0;}
        if(!strcmp(method,"POST")&&!strcmp(path,"/api/queue")){add_queue(c,json_int(body,"id",-1));return 0;}
        if(!strcmp(method,"POST")&&!strcmp(path,"/api/queue/process")){process_queue(c);return 0;}
        if(!strcmp(method,"DELETE")&&!strcmp(path,"/api/services")){while(records){Record*n=records->next;free(records);records=n;}tail=NULL;while(qfront){QNode*n=qfront->next;free(qfront);qfront=n;}qrear=NULL;persist();send_json(c,200,"{\"ok\":true}");return 0;}
        if(!strcmp(method,"DELETE")&&!strcmp(path,"/api/queue")){while(qfront){QNode*n=qfront->next;free(qfront);qfront=n;}qrear=NULL;persist();send_json(c,200,"{\"ok\":true}");return 0;}
        int id=0;char extra[32]="";if(sscanf(path,"/api/services/%d/%31s",&id,extra)>=2&&!strcmp(method,"PATCH")&&!strcmp(extra,"status")){update_status(c,id,body);return 0;}
        if(sscanf(path,"/api/services/%d",&id)==1&&!strchr(path+14,'/')&&!strcmp(method,"DELETE")){delete_record(c,id);return 0;}
        send_json(c,404,"{\"error\":\"API endpoint not found\"}");return 0;
    }
    if(strcmp(method,"GET")){send_response(c,405,"text/plain","Method not allowed");return 0;}serve_static(c,path);return 0;
}
int main(void){
#ifdef _WIN32
    WSADATA wsa;if(WSAStartup(MAKEWORD(2,2),&wsa)!=0){fprintf(stderr,"Winsock initialization failed.\n");return 1;}
#endif
    load_data();socket_t server=socket(AF_INET,SOCK_STREAM,0);if(server==INVALID_SOCKET){fprintf(stderr,"Could not create server socket.\n");return 1;}int opt=1;
#ifndef _WIN32
    setsockopt(server,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof opt);
#endif
    struct sockaddr_in addr;memset(&addr,0,sizeof addr);addr.sin_family=AF_INET;addr.sin_addr.s_addr=htonl(INADDR_ANY);addr.sin_port=htons(PORT);
    if(bind(server,(struct sockaddr*)&addr,sizeof addr)==SOCKET_ERROR||listen(server,16)==SOCKET_ERROR){fprintf(stderr,"Could not start server on port %d. Is it already in use?\n",PORT);CLOSESOCKET(server);return 1;}
    printf("\nVehicle Service Tracking System - C backend\nOpen http://localhost:%d in your browser.\nPress Ctrl+C to stop the server.\n\n",PORT);
    for(;;){struct sockaddr_in clientAddr;
#ifdef _WIN32
        int len=sizeof clientAddr;
#else
        socklen_t len=sizeof clientAddr;
#endif
        socket_t client=accept(server,(struct sockaddr*)&clientAddr,&len);if(client==INVALID_SOCKET)continue;char*req=calloc(1,REQ_SIZE+1);if(!req){CLOSESOCKET(client);continue;}int total=0,n;while(total<REQ_SIZE&&(n=recv(client,req+total,REQ_SIZE-total,0))>0){total+=n;req[total]=0;char*h=strstr(req,"\r\n\r\n");if(h){int contentLen=0;char*cl=strstr(req,"Content-Length:");if(!cl)cl=strstr(req,"content-length:");if(cl)contentLen=atoi(cl+15);int have=(int)(req+total-(h+4));if(have>=contentLen)break;}}
        if(total>0)handle_request(client,req);free(req);CLOSESOCKET(client);
    }
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
