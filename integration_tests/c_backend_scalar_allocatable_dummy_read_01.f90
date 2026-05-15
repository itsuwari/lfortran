program c_backend_scalar_allocatable_dummy_read_01
    implicit none

    real(8), allocatable :: charge

    call parse_charge("1.25", charge)

    if (.not. allocated(charge)) error stop "charge not allocated"
    if (abs(charge - 1.25d0) > 1.0d-12) error stop "wrong charge"

contains

    subroutine parse_charge(arg, charge)
        character(len=*), intent(in) :: arg
        real(8), allocatable, intent(out) :: charge
        integer :: stat

        allocate(charge)
        read(arg, *, iostat=stat) charge
        if (stat /= 0) error stop "read failed"
    end subroutine

end program
