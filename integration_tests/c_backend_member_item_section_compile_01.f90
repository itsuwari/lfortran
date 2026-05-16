program c_backend_member_item_section_compile_01
implicit none

type shell_t
    real(8) :: alpha(3)
    integer :: nprim
end type

type(shell_t), allocatable :: cgto(:, :)
real(8) :: value, expected
integer :: i, ish, isp

allocate(cgto(2, 2))
do ish = 1, 2
    do isp = 1, 2
        cgto(ish, isp)%nprim = 2
        do i = 1, 3
            cgto(ish, isp)%alpha(i) = real(10*ish + 3*isp + i, 8)
        end do
    end do
end do

ish = 2
isp = 1
value = minval(cgto(ish, isp)%alpha(:cgto(ish, isp)%nprim))
expected = min(cgto(ish, isp)%alpha(1), cgto(ish, isp)%alpha(2))

if (abs(value - expected) > 1.0e-12_8) error stop

end program
