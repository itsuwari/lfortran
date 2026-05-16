program allocatable_struct_array_cleanup_01
implicit none

type item
    integer, allocatable :: values(:)
end type

call exercise(3)
call exercise(5)

contains

subroutine exercise(n)
    integer, intent(in) :: n
    type(item), allocatable :: bucket(:)
    integer :: i

    allocate(bucket(n))
    do i = 1, n
        allocate(bucket(i)%values(2))
        bucket(i)%values = [i, i + 1]
    end do
    if (sum(bucket(n)%values) /= 2*n + 1) error stop

    deallocate(bucket)
    allocate(bucket(1))
    allocate(bucket(1)%values(1))
    bucket(1)%values(1) = 42
    if (bucket(1)%values(1) /= 42) error stop
end subroutine

end program
