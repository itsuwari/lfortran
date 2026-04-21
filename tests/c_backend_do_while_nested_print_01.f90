program c_backend_do_while_nested_print_01
    implicit none

    integer :: stat, marker, mode
    character(len=:), allocatable :: line, msg

    stat = 0
    marker = 0
    mode = 1
    line = "$cell"
    msg = "ok"

    do while (stat == 0)
        if (index(line, "$") == 1) then
            select case (mode)
            case (1)
                print *, msg
                marker = 1
            end select
        end if
        stat = 1
    end do

    if (marker /= 1) error stop 1
    print *, marker
end program c_backend_do_while_nested_print_01
