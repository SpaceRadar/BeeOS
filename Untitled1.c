            //no one task is waiting this message 
            if( ((mailbox_t*)((send_message_t*)param)->handle)->counter< ((mailbox_t*)((send_message_t*)param)->handle)->maxmsg )
            {
              ((mailbox_t*)((send_message_t*)param)->handle)->write_packet->size=((send_message_t*)param)->size;
              memcpy( ((mailbox_t*)((send_message_t*)param)->handle)->write_packet->data,
                      ((send_message_t*)param)->buffer,
                      ((send_message_t*)param)->size); 

              //Move write pointer
              if(((mailbox_t*)((send_message_t*)param)->handle)->write_idx < ((mailbox_t*)((send_message_t*)param)->handle)->maxmsg)
              {    
                //move write pointer on one step
                ((mailbox_t*)((send_message_t*)param)->handle)->write_packet= (mailbox_packet_t*)
                  (((uint32_t) ((mailbox_t*)((send_message_t*)param)->handle)->write_packet)
                  +((mailbox_t*)((send_message_t*)param)->handle)->paket_size);
                ++((mailbox_t*)((send_message_t*)param)->handle)->write_idx;                
              }
              else
              {
                //move write pointer to first position              
                ((mailbox_t*)((send_message_t*)param)->handle)->write_packet=((mailbox_t*)((send_message_t*)param)->handle)->buffer;
                ((mailbox_t*)((send_message_t*)param)->handle)->write_idx=0;              
              }
                
              ++((mailbox_t*)((send_message_t*)param)->handle)->counter;             
            }          
          else
          {
            current_task->State=WAITING_HANDLE;
            ReadyTasksCurrent&=~(0x80000000>>current_task_id);
            ReadyTasks&=~(0x80000000>>current_task_id);              
            ((mailbox_t*)((send_message_t*)param)->handle)->write_waiting_tasks|=(0x80000000>>current_task_id);
            result=0xFFFFFFFF; 
          }

            
          }
          ((send_message_t*) param)->result=0;   





          task_id=__CLZ(((mailbox_t*)((send_message_t*)param)->handle)->read_waiting_tasks & ~SleepTasks);
          if(task_id<32)
          {
            //same task is waiting this message 
            if( ((get_message_t*)TCB[task_id]->RequestStruct)->size<= ((get_message_t*)param)->size ) 
            {                        
                memcpy( ((mailbox_t*)((send_message_t*)param)->handle)->write_packet->data,
                        ((get_message_t*)TCB[task_id]->RequestStruct)->buffer,
                        ((send_message_t*)param)->size);             
                ((get_message_t*)TCB[task_id]->RequestStruct)->result=((send_message_t*)param)->size;
            }
            else
            {
                //buffer too small
                ((get_message_t*)TCB[task_id]->RequestStruct)->result=((send_message_t*)param)->size;              
            }            
            ((mailbox_t*)((send_message_t*)param)->handle)->read_waiting_tasks&=~(0x80000000>>task_id);    
            ReadyTasksCurrent|=(0x80000000>>task_id);
            ReadyTasks|=(0x80000000>>task_id);              
          }  

