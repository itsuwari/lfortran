module c_backend_split_common_header_01
implicit none

type :: point_t
    real :: x, y
end type

contains
    subroutine set_point(p, x, y)
        type(point_t), intent(out) :: p
        real, intent(in) :: x, y
        p%x = x
        p%y = y
    end subroutine set_point

    subroutine clear_values(values)
        real, intent(out) :: values(:)
        values = 0.0
    end subroutine clear_values
end module
