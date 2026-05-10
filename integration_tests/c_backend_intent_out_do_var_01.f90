subroutine find_index(n, info)
    integer, intent(in) :: n
    integer, intent(out) :: info

    do info = n, 1, -1
        if (info == 2) return
    end do
end subroutine

program main
    integer :: info

    call find_index(4, info)
    if (info /= 2) error stop
end program
