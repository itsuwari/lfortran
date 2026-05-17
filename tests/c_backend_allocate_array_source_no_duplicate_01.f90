program c_backend_allocate_array_source_no_duplicate_01
implicit none

integer :: i, j
real(8), allocatable :: src(:, :), dst(:, :)

allocate(src(2, 3))
do j = 1, 3
    do i = 1, 2
        src(i, j) = 10.0_8*i + j
    end do
end do

allocate(dst, source=src)

if (size(dst, 1) /= 2) error stop
if (size(dst, 2) /= 3) error stop
if (dst(1, 1) /= src(1, 1)) error stop
if (dst(2, 3) /= src(2, 3)) error stop

end program
