module c_backend_local_fixed_array_wrapper_01
implicit none
contains

    subroutine takes_array(x)
        real(8), intent(inout) :: x(:)

        x(1) = x(1) + 1d0
    end subroutine takes_array

    subroutine run_case()
        real(8) :: a(3)

        a = [1d0, 2d0, 3d0]
        call takes_array(a)
        print '(3F0.1)', a
    end subroutine run_case

end module c_backend_local_fixed_array_wrapper_01

program test_c_backend_local_fixed_array_wrapper_01
    use c_backend_local_fixed_array_wrapper_01
    implicit none

    call run_case()
end program test_c_backend_local_fixed_array_wrapper_01
