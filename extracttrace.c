#include "math.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "msg/msg.h"            /* Yeah! If you want to use msg, you need to include msg/msg.h */
#include <xbt/replay.h>
/* Create a log channel to have nice outputs. */
#include "xbt.h"
#include<stdlib.h>
#include "xbt/log.h"
#include "xbt/sysdep.h"
#include "xbt/asserts.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(replay, "Messages specific for this example");
static xbt_dict_t opened_files = NULL;

#define ACT_DEBUG(...) \
  if (XBT_LOG_ISENABLED(replay, xbt_log_priority_verbose)) {  \
    char *NAME = xbt_str_join_array(action, " ");              \
    XBT_DEBUG(__VA_ARGS__);                                    \
    xbt_free(NAME);                                            \
  } else ((void)0)

static void log_action(const char *const *action, double date)
{
  if(XBT_LOG_ISENABLED(replay, xbt_log_priority_verbose)) {
    char *name = xbt_str_join_array(action, " ");
    XBT_VERB("%s %.9f", name, date);
    xbt_free(name);
  }
}

msg_file_t get_file_descriptor(const char *file_name,const char *index){//get file with the file full_name
  char full_name[1024];
  msg_file_t file = NULL;
  sprintf(full_name, "%s:%s:%s", file_name,MSG_process_get_name(MSG_process_self()),index);
  file = (msg_file_t) xbt_dict_get_or_null(opened_files, full_name);
 return file;
}

static sg_size_t parse_size(const char *string){
  sg_size_t size;
  char *endptr;
  size = strtoul(string, &endptr, 10);
  if (*endptr != '\0')
  {  THROWF(unknown_error, 0, "%s is not a long unsigned int (a.k.a. sg_size_t)", string);
  }  
  return size;
}
static sg_offset_t parse_offset(const char *string){
  sg_offset_t offset;
  offset = atol(string);
  return offset;
}
void simsleep(const char *const *action) {//the sleep function that creats and executes the task which only sleep
  const char *processid = action[0];
  const char *worker = action[1];
  const char *worktime = action[3];
  double sleeptime;
  double clock = MSG_get_clock();       /* this "call" is free thanks to inlining */
  sleeptime = atof(worktime); 
  MSG_process_sleep(sleeptime); 
  log_action(action, MSG_get_clock() - clock);
  XBT_INFO("%s  worker %s is done,time is %s",worker,processid,worktime);
}

static void simopen(const char *const *action) {
  const char *processid = action[0];
  const char *file_name = action[2];
  const char *index = action[4];
  char full_name[1024];
  msg_file_t file = NULL;
  double clock = MSG_get_clock();       /* this "call" is free thanks to inlining */
  sprintf(full_name, "%s:%s:%s", file_name,MSG_process_get_name(MSG_process_self()),index);
  ACT_DEBUG("Entering Open: %s (filename: %s)", NAME, file_name);
  file = MSG_file_open(full_name, NULL);
  xbt_dict_set(opened_files, full_name, file, NULL);
  log_action(action, MSG_get_clock() - clock);
  XBT_INFO("open worker %s%s is done",processid,index);
}
static void simrelease(const char *const *action) {
  const char *processid = action[0];
  const char *file_name = action[2];
  const char *index = action[4];
  msg_file_t file;
  double clock = MSG_get_clock();       /* this "call" is free thanks to inlining */
  file = get_file_descriptor(file_name,index);
  ACT_DEBUG("Entering Close: %s (filename: %s)", NAME, file_name);
  MSG_file_close(file);
  log_action(action, MSG_get_clock() - clock);
  XBT_INFO("release worker %s%s is done",processid,index);
}

static void simread(const char *const *action) {
  const char *processid = action[0];
  const char *file_name = action[2];
  const char *index = action[4];
  const char *position_str =action[5];
  const char *size_str = action[6];
  msg_file_t file = NULL;
  sg_size_t size = parse_size(size_str);
  sg_offset_t position= parse_offset(position_str);
  double clock = MSG_get_clock();       /* this "call" is free thanks to inlining */
  file = get_file_descriptor(file_name,index);
  ACT_DEBUG("Entering Read: %s (size: %llu)", NAME, size);
  MSG_file_seek(file,position,SEEK_SET);
  MSG_file_read(file, size);
  log_action(action, MSG_get_clock() - clock);
  XBT_INFO("read  worker %s%s is done",processid,index);
}
static void simcreat(const char *const *action) {
  const char *processid = action[0];
  const char *file_name = action[2];
  const char *index = action[4];
  char full_name[1024];
  msg_file_t file = NULL;
  double clock = MSG_get_clock();       /* this "call" is free thanks to inlining */
  sprintf(full_name, "%s:%s:%s", file_name,MSG_process_get_name(MSG_process_self()), index);
  ACT_DEBUG("Entering Creat: %s (filename: %s)", NAME, file_name);
  file = MSG_file_open(full_name, NULL);
  log_action(action, MSG_get_clock() - clock);
  XBT_INFO("creat worker %s %s is done",processid,index);
  xbt_dict_set(opened_files, full_name, file, NULL);
}
static void simwrite(const char *const *action) {
  const char *processid = action[0];
  const char *file_name = action[2];
  const char *index = action[4];
  const char *position_str =action[5];
  const char *size_str = action[6];
  msg_file_t file = NULL;
  sg_size_t size = parse_size(size_str);
  sg_offset_t position= parse_offset(position_str);
  double clock = MSG_get_clock();       /* this "call" is free thanks to inlining */
  file = get_file_descriptor(file_name,index);
  ACT_DEBUG("Entering Write: %s (size: %llu)", NAME, size);
  MSG_file_seek(file,position,SEEK_SET);//seek the position to write 
  MSG_file_write(file, size);//write to the file
  XBT_INFO("write worker %s%s is done",processid,index);
  log_action(action, MSG_get_clock() - clock);
}
int main(int argc, char *argv[]) {
  /* Check the given arguments */
  MSG_init(&argc, argv);
  /* Explicit initialization of the action module is required now*/
  MSG_action_init();
  MSG_config("surf/precision","1e-9");
  MSG_create_environment(argv[1]);
 /*  Simulation setting */
  msg_error_t res = MSG_OK;

  /* No need to register functions as in classical MSG programs: the actions get started anyway */
  MSG_launch_application(argv[2]);
  char *wh=argv[4];
  XBT_INFO("Your choose is %s",wh);
  xbt_replay_action_register("unlink", simsleep);//register the action
  xbt_replay_action_register("access", simsleep);
  xbt_replay_action_register("stat", simsleep);
  xbt_replay_action_register("flush", simsleep);
  xbt_replay_action_register("readdir", simsleep);
  xbt_replay_action_register("getxattr", simsleep);
  xbt_replay_action_register("mkdir", simsleep);
  xbt_replay_action_register("symlink", simsleep);
  xbt_replay_action_register("readlink", simsleep);
  if(strcmp(wh,"action")==0) //decide which mode to run
  /*   Action registration */
 {
  xbt_replay_action_register("open", simopen);
  xbt_replay_action_register("release", simrelease);
  xbt_replay_action_register("read", simread);
  xbt_replay_action_register("creat", simcreat);
  xbt_replay_action_register("write", simwrite);
 }
  else if(strcmp(wh,"sleep")==0){
  xbt_replay_action_register("open", simsleep);
  xbt_replay_action_register("release", simsleep);
  xbt_replay_action_register("read", simsleep);
  xbt_replay_action_register("creat", simsleep);
  xbt_replay_action_register("write", simsleep);
  }
 
  if(!opened_files)
  {opened_files = xbt_dict_new_homogeneous(NULL);}
  res = MSG_action_trace_run(argv[3]);  // it's ok to pass a NULL argument here

  XBT_INFO("Simulation time %g", MSG_get_clock());
  if(opened_files)
	{xbt_dict_free(&opened_files);}
 MSG_action_exit();
  return !!MSG_OK;
}
