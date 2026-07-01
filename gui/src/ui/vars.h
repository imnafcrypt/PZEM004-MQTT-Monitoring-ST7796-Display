#ifndef EEZ_LVGL_UI_VARS_H
#define EEZ_LVGL_UI_VARS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// enum declarations

// Flow global variables

enum FlowGlobalVariables {
    FLOW_GLOBAL_VARIABLE_VOLT = 0,
    FLOW_GLOBAL_VARIABLE_AMPERE = 1,
    FLOW_GLOBAL_VARIABLE_WATT = 2,
    FLOW_GLOBAL_VARIABLE_WATTHOUR = 3,
    FLOW_GLOBAL_VARIABLE_HERTZ = 4,
    FLOW_GLOBAL_VARIABLE_POWERFACTOR = 5,
    FLOW_GLOBAL_VARIABLE_NETWORK = 6
};

// Native global variables

extern float get_var_volt();
extern void set_var_volt(float value);
extern float get_var_ampere();
extern void set_var_ampere(float value);
extern float get_var_watt();
extern void set_var_watt(float value);
extern float get_var_watthour();
extern void set_var_watthour(float value);
extern float get_var_hertz();
extern void set_var_hertz(float value);
extern float get_var_powerfactor();
extern void set_var_powerfactor(float value);
extern bool get_var_network();
extern void set_var_network(bool value);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/