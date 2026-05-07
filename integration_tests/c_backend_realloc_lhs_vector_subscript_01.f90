program c_backend_realloc_lhs_vector_subscript_01
    implicit none

    integer, allocatable :: list(:, :)

    call fill_list(list)
    if (any(list(:, 1) /= [11, 22, 0, 44, 0])) error stop
    if (any(list(:, 2) /= [0, 0, 33, 0, 55])) error stop

contains

    subroutine fill_list(list)
        integer, allocatable, intent(out) :: list(:, :)
        integer :: n, a, b, c, d, e

        allocate(list(5, 2))
        list = 0

        n = 1
        a = 11
        b = 22
        c = 44
        list([1, 2, 4], n) = [a, b, c]

        n = 2
        d = 33
        e = 55
        list([3, 5], n) = [d, e]
    end subroutine
end program
