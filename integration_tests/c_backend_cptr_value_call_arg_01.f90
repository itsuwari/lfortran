module c_backend_cptr_value_call_arg_01_mod
    use, intrinsic :: iso_c_binding
    implicit none

    interface
        function c_strlen(str) result(n) bind(c, name="strlen")
            import :: c_ptr, c_int
            type(c_ptr), value :: str
            integer(c_int) :: n
        end function
    end interface

contains

    subroutine check_c_strlen(ptr, run_call)
        type(c_ptr), value :: ptr
        logical, intent(in) :: run_call
        integer(c_int) :: n

        n = 0_c_int
        if (run_call) then
            n = c_strlen(ptr)
        end if
        if (n /= 0_c_int) error stop "unexpected strlen call"
    end subroutine

end module

program c_backend_cptr_value_call_arg_01
    use, intrinsic :: iso_c_binding, only: c_null_ptr
    use c_backend_cptr_value_call_arg_01_mod, only: check_c_strlen
    implicit none

    call check_c_strlen(c_null_ptr, .false.)
end program
