module c_backend_fixed_member_array_actual_01
    implicit none

    type :: cgto_type
        real(8) :: alpha(4) = 0.0d0
        real(8) :: coeff(4) = 0.0d0
    end type cgto_type

contains

    subroutine fill_array(alpha, coeff)
        real(8), intent(out) :: alpha(:)
        real(8), intent(out) :: coeff(:)

        alpha(:) = [1.0d0, 2.0d0, 3.0d0, 4.0d0]
        coeff(:) = [5.0d0, 6.0d0, 7.0d0, 8.0d0]
    end subroutine fill_array

    subroutine fill_cgto(cgto)
        type(cgto_type), intent(out) :: cgto

        call fill_array(cgto%alpha, cgto%coeff)
    end subroutine fill_cgto

end module c_backend_fixed_member_array_actual_01

program test_c_backend_fixed_member_array_actual_01
    use c_backend_fixed_member_array_actual_01, only: cgto_type, fill_cgto
    implicit none

    type(cgto_type) :: cgto

    call fill_cgto(cgto)
    print "(4F0.6)", cgto%alpha
    print "(4F0.6)", cgto%coeff
end program test_c_backend_fixed_member_array_actual_01
