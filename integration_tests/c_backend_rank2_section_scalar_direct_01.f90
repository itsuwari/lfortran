program c_backend_rank2_section_scalar_direct_01
implicit none

real(8) :: a(4, 5)
integer :: i, j

a = -1.0d0
a(2:4, 2:4) = 3.5d0

do j = 1, 5
    do i = 1, 4
        if (i >= 2 .and. j >= 2 .and. j <= 4) then
            if (abs(a(i, j) - 3.5d0) > 1.0d-12) error stop
        else
            if (abs(a(i, j) + 1.0d0) > 1.0d-12) error stop
        end if
    end do
end do

end program c_backend_rank2_section_scalar_direct_01
