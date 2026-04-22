module bindc_51_mod
    use, intrinsic :: iso_c_binding, only: c_int
    implicit none

    character(len=*), parameter :: namespace = "bindc_51_"

contains

    integer(c_int) function get_value() result(value) bind(c, name=namespace // "get_value")
        value = 51_c_int
    end function get_value

end module bindc_51_mod

program bindc_51
    use bindc_51_mod, only: get_value
    implicit none

    if (get_value() /= 51) error stop
    print *, "bindc_51 ok"
end program bindc_51
