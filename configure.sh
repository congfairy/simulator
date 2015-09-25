#!/bin/bash
filename=$1
fileactiontrace=${filename%.*}"_action_trace.txt"
filecontent="storage_content.txt"
filedeployment=${filename%.*}"_deployment.xml"
awk -F',' '/file,open/ {gsub("/home","",$10);print $10" "$15}' $filename  | sort -u -k1,1 > $filecontent

printf "<?xml version='1.0'?>\n<!DOCTYPE platform SYSTEM \"http://simgrid.gforge.inria.fr/simgrid.dtd\">\n<platform version=\"3\">\n" >$filedeployment
starttime=0;
awk -F',' '/file/ && !d[$9]++ && $9!=0 {if(starttime==0)
{starttime=$1;
}
else{
   gsub("Z","",$1);
   gsub("T"," ",$1);
   gsub("Z","",starttime);
   gsub("T"," ",starttime);
   cmd="echo $(date -d \""$1"\" +%s%N) - $(date -d \""starttime"\" +%s%N) | bc";
   cmd | getline var
};
printf(" <process host=\"denise\" function=\"%s\" start_time=\"%.9f\"/>\n",$9,(var/1e9))}'  $filename >> $filedeployment 
echo "</platform>" >> $filedeployment

awk -F ',' '/file/' $filename|awk -F',' '
function compare(action,pid,filepath)
{
if(action=="open"||action=="creat")
{
fileopen[fno++]=pid;
fileopenpath[gno++]=filepath;
}
if(action=="release"&&pid=="0")
{
for(k=0;k<fno;k++)
{
if(filepath==fileopenpath[k])
 {
   gsub("0",fileopen[k],pid);
  for(h=k;h<fno-1;h++)
  {
        fileopen[h]=fileopen[h+1];
	fileopenpath[h]=fileopenpath[h+1];
  }
  fno=fno-1;
  gno=gno-1;
break;
}
}
}
return pid;
}
function compute(starttime,endtime,pid,filepath)
{
gsub("home",pid,filepath);
find=0;
gsub("Z","",starttime);
gsub("T"," ",starttime);
gsub("Z","",endtime);
gsub("T"," ",endtime);
for(i=0;i<no;i++)
{
if(filepath==filestr[i])
{
find=1;
cmd="echo $(date -d \""starttime"\" +%s%N) - $(date -d \""timestr[i]"\" +%s%N) | bc";
cmd | getline var;
close(cmd);
filestr[i]=filepath;
timestr[i]=endtime;
}
}
if(find==0){
filestr[no++]=filepath;
timestr[tno++]=endtime;
var=0;
}
return var; 
}
{
ppid=compare($12,$9,$10);
dur=compute($1,$2,ppid,$10);
if(dur!=0)
{printf("%s %s %s %.9f\n",ppid,"compute",$10,dur*(1e-9))}
if($12=="read")
{printf("%s %s %s %.9f %s %s %s\n",ppid,$12,$10,$3*(1e-9),$17,$14,$15)}
else if($12=="write")
{printf("%s %s %s %.9f %s %s %s\n",ppid,$12,$10,$3*(1e-9),$16,$13,$14)}
else if($12=="open")
{printf("%s %s %s %.9f %s\n",ppid,$12,$10,$3*(1e-9),$17)}
else if($12=="creat"||$12=="flush")
{printf("%s %s %s %.9f %s\n",ppid,$12,$10,$3*(1e-9),$15)}
else if($12=="release")
{printf("%s %s %s %.9f %s\n",ppid,$12,$10,$3*(1e-9),$13)}
else
{printf("%s %s %s %.9f\n",ppid,$12,$10,$3*(1e-9))}
}'  >$fileactiontrace
