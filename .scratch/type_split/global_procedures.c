#include "type_generated.h"

bool __lfortran__lcompilers_Any_4_1_0_logical____0(struct l32___* mask, int32_t __1mask)
{
    int32_t __1_i;
    bool _lcompilers_Any_4_1_0;
    _lcompilers_Any_4_1_0 = false;
    for (__1_i=1; __1_i<=__1mask + 1 - 1; __1_i++) {
        _lcompilers_Any_4_1_0 = _lcompilers_Any_4_1_0 || mask->data[(0 + (1 * (__1_i - 1)))];
    }
    return _lcompilers_Any_4_1_0;
}

;


;


void __lfortran__lcompilers_get_command_argument_(int32_t number, int32_t *length, int32_t *status)
{
    int32_t length_to_allocate;
    int32_t stat;
    length_to_allocate = 0;
    length_to_allocate = _lfortran_get_command_argument_length(number);
    (*length) = length_to_allocate;
    stat = _lfortran_get_command_argument_status(number, length_to_allocate, length_to_allocate);
    (*status) = stat;
}

;


;


;


void __lfortran__lcompilers_get_command_argument_1(int32_t number, char * *value, int32_t *status)
{
    char * command_argument_holder = NULL;
    int32_t length_to_allocate;
    int32_t stat;
    length_to_allocate = 0;
    length_to_allocate = _lfortran_get_command_argument_length(number);
    command_argument_holder = (char*) _lfortran_string_malloc_alloc(_lfortran_get_default_allocator(), length_to_allocate);
    _lfortran_get_command_argument_value(number, command_argument_holder);
    _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &(*value), NULL, true, true, command_argument_holder, strlen(command_argument_holder));
    stat = _lfortran_get_command_argument_status(number, strlen((*value)), length_to_allocate);
    (*status) = stat;
}

;


;


void __lfortran__lcompilers_get_environment_variable_(char * name, int32_t *length, int32_t *status)
{
    (*length) = _lfortran_get_length_of_environment_variable(name, strlen(name));
    (*status) = _lfortran_get_environment_variable_status(name, strlen(name));
}

;


;


;


void __lfortran__lcompilers_get_environment_variable_1(char * name, char * *value, int32_t *status)
{
    char * envVar_string_holder = NULL;
    int32_t length_to_allocate;
    length_to_allocate = _lfortran_get_length_of_environment_variable(name, strlen(name));
    envVar_string_holder = (char*) _lfortran_string_malloc_alloc(_lfortran_get_default_allocator(), length_to_allocate);
    _lfortran_get_environment_variable(name, strlen(name), envVar_string_holder);
    _lfortran_strcpy_alloc(_lfortran_get_default_allocator(), &(*value), NULL, true, true, envVar_string_holder, strlen(envVar_string_holder));
    // FIXME: deallocate(_lcompilers_get_environment_variable_1__envVar_string_holder, );
    (*status) = _lfortran_get_environment_variable_status(name, strlen(name));
}

int32_t __lfortran__lcompilers_index_Allocatable_x5B_str_x5D_(char * str, char * substr, bool back, int32_t kind)
{
    int32_t _lcompilers_index_Allocatable_x5B_str_x5D_;
    bool found;
    int32_t i;
    int32_t j;
    int32_t k;
    int32_t pos;
    _lcompilers_index_Allocatable_x5B_str_x5D_ = 0;
    i = 1;
    found = true;
    if (strlen(str) < strlen(substr)) {
        found = false;
    }
    while (i < strlen(str) + 1 && found == true) {
        k = 0;
        j = 1;
        while (j <= strlen(substr) && found == true) {
            pos = i + k;
            if (str_compare(_lfortran_str_slice_alloc(_lfortran_get_default_allocator(), str, strlen(str), ((pos) - 1), ((1) > 0 ? (pos) : ((pos) - 2)), 1, true, true), pos - pos + 1, _lfortran_str_slice_alloc(_lfortran_get_default_allocator(), substr, strlen(substr), ((j) - 1), ((1) > 0 ? (j) : ((j) - 2)), 1, true, true), j - j + 1)  !=  0) {
                found = false;
            }
            j = j + 1;
            k = k + 1;
        }
        if (found == true) {
            _lcompilers_index_Allocatable_x5B_str_x5D_ = i;
            found = back;
        } else {
            found = true;
        }
        i = i + 1;
    }
    return _lcompilers_index_Allocatable_x5B_str_x5D_;
}

int32_t __lfortran__lcompilers_index_Allocatable_x5B_str_x5D_1(char * str, char * substr, bool back, int32_t kind)
{
    int32_t _lcompilers_index_Allocatable_x5B_str_x5D_1;
    bool found;
    int32_t i;
    int32_t j;
    int32_t k;
    int32_t pos;
    _lcompilers_index_Allocatable_x5B_str_x5D_1 = 0;
    i = 1;
    found = true;
    if (strlen(str) < strlen(substr)) {
        found = false;
    }
    while (i < strlen(str) + 1 && found == true) {
        k = 0;
        j = 1;
        while (j <= strlen(substr) && found == true) {
            pos = i + k;
            if (str_compare(_lfortran_str_slice_alloc(_lfortran_get_default_allocator(), str, strlen(str), ((pos) - 1), ((1) > 0 ? (pos) : ((pos) - 2)), 1, true, true), pos - pos + 1, _lfortran_str_slice_alloc(_lfortran_get_default_allocator(), substr, strlen(substr), ((j) - 1), ((1) > 0 ? (j) : ((j) - 2)), 1, true, true), j - j + 1)  !=  0) {
                found = false;
            }
            j = j + 1;
            k = k + 1;
        }
        if (found == true) {
            _lcompilers_index_Allocatable_x5B_str_x5D_1 = i;
            found = back;
        } else {
            found = true;
        }
        i = i + 1;
    }
    return _lcompilers_index_Allocatable_x5B_str_x5D_1;
}

int32_t __lfortran__lcompilers_index_Allocatable_x5B_str_x5D_2(char * str, char * substr, bool back, int32_t kind)
{
    int32_t _lcompilers_index_Allocatable_x5B_str_x5D_2;
    bool found;
    int32_t i;
    int32_t j;
    int32_t k;
    int32_t pos;
    _lcompilers_index_Allocatable_x5B_str_x5D_2 = 0;
    i = 1;
    found = true;
    if (strlen(str) < strlen(substr)) {
        found = false;
    }
    while (i < strlen(str) + 1 && found == true) {
        k = 0;
        j = 1;
        while (j <= strlen(substr) && found == true) {
            pos = i + k;
            if (str_compare(_lfortran_str_slice_alloc(_lfortran_get_default_allocator(), str, strlen(str), ((pos) - 1), ((1) > 0 ? (pos) : ((pos) - 2)), 1, true, true), pos - pos + 1, _lfortran_str_slice_alloc(_lfortran_get_default_allocator(), substr, strlen(substr), ((j) - 1), ((1) > 0 ? (j) : ((j) - 2)), 1, true, true), j - j + 1)  !=  0) {
                found = false;
            }
            j = j + 1;
            k = k + 1;
        }
        if (found == true) {
            _lcompilers_index_Allocatable_x5B_str_x5D_2 = i;
            found = back;
        } else {
            found = true;
        }
        i = i + 1;
    }
    return _lcompilers_index_Allocatable_x5B_str_x5D_2;
}

