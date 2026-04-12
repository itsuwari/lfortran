module bindc_46_mod
    use, intrinsic :: iso_c_binding, only: c_int
    implicit none

    character(len=*), parameter :: namespace = "bindc_46_"

contains

    integer(c_int) function get_value() result(value) bind(c, name=namespace // "get_value")
        value = 46_c_int
    end function get_value

end module bindc_46_mod

program bindc_46
    use bindc_46_mod, only: get_value
    implicit none

    if (get_value() /= 46) error stop
    print *, "bindc_46 ok"
end program bindc_46
