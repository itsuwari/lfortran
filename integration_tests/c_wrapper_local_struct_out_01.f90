module c_wrapper_local_struct_out_01
    type :: t
        integer :: n
    end type
contains

    subroutine init(self)
        type(t), intent(out) :: self

        self%n = 1
    end subroutine

    subroutine test_out()
        type(t) :: x

        call init(x)
        if (x%n /= 1) error stop 1
    end subroutine

end module

program main
    use c_wrapper_local_struct_out_01

    call test_out()
end program
