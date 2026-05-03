module c_backend_submodule_proc_name_01_m
    implicit none

    abstract interface
        subroutine callback(x)
            integer, intent(out) :: x
        end subroutine
    end interface

    interface
        module subroutine worker(x)
            integer, intent(out) :: x
        end subroutine
    end interface

contains

    subroutine call_callback(proc, x)
        procedure(callback) :: proc
        integer, intent(out) :: x
        call proc(x)
    end subroutine

    subroutine collect(x)
        integer, intent(out) :: x
        call call_callback(worker, x)
    end subroutine

end module

submodule (c_backend_submodule_proc_name_01_m) c_backend_submodule_proc_name_01_s
contains

    module subroutine worker(x)
        integer, intent(out) :: x
        x = 42
    end subroutine

end submodule

program c_backend_submodule_proc_name_01
    use c_backend_submodule_proc_name_01_m, only: collect
    implicit none
    integer :: x
    call collect(x)
    if (x /= 42) error stop
end program
