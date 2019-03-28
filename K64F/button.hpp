/**
* copyright (c) 2018, James Flynn
* SPDX-License-Identifier: MIT
*/

/**
*   @file   button.hpp
*   @brief  A small BUTTON class for detecting & debouncing button preesses
*
*   @author James Flynn
*
*   @date   24-Aug-2018
*/

#include "mbed.h"

#define BUTTON_DEBOUNCE           20     //specify the number of msec to debounce

class Button {
    protected:
        InterruptIn user_button;
        void (*br_cb)(int);    //button release callback
        void (*bp_cb)(void);   //button press callback

        Thread      *button_thread;
        void        button_monitor_task(void);
        EventQueue  button_queue;
        uint64_t    bp_time, bp_duration;  //button press start time and button press duration
        int         button_pressed;        //counts the number of times the button has been pressed

        void button_press_handler(void) {
            if( (rtos::Kernel::get_ms_count() - bp_time) < BUTTON_DEBOUNCE)
                return;
            bp_time = rtos::Kernel::get_ms_count();
            if( bp_cb )
                bp_cb();
        }

        void button_release_handler(void) {
            uint64_t tmp = rtos::Kernel::get_ms_count() - bp_time;
            if( tmp > BUTTON_DEBOUNCE ) {
                bp_duration = tmp;
                button_pressed++;
                if( br_cb )
                  br_cb(bp_duration);
                }
        }

    public:
        enum State { ActiveHigh = 0, ActiveLow };

        Button(PinName p, State s, void (*cb)(int)=NULL) : 
            user_button(p),
            br_cb(cb),
            bp_cb(NULL),
            bp_time(0),
            bp_duration(0),
            button_pressed(0)
            {
            // The user button is setup for the edge to generate an interrupt. 
            // The release is caught an event queue callback
            button_thread=new Thread(osPriorityNormal,256,NULL,"button_thread");
            button_thread->start(callback(&button_queue, &EventQueue::dispatch_forever));
            if( s == ActiveHigh ) {
                user_button.rise( Callback<void()>(this, &Button::button_press_handler) ); 
                user_button.fall( button_queue.event( Callback<void()>(this, &Button::button_release_handler)));
                }
            else{
                user_button.fall( Callback<void()>(this, &Button::button_press_handler) );
                user_button.rise(button_queue.event(Callback<void()>(this, &Button::button_release_handler)));
                }
            }

        ~Button() {
            button_thread->terminate();
            delete button_thread;
            }

        // will return the number of times the button has been pressed (if it was pressed > 1 time before checked)
        // and returns the duration of the last button press in duration
        int chkButton_press(int *duration) {
            int bp = button_pressed;
        
            if( button_pressed ) {
                *duration = bp_duration;
                bp_duration = 0;
                button_pressed = 0;
                }
            return bp;
            }
            
        //allows the user to set a callback for a button press in
        void setButton_press_cb( void (*buttonpresscb)(void) ) {
            bp_cb = buttonpresscb;
            }
};

