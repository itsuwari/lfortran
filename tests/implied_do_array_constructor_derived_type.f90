program implied_do_array_constructor_derived_type
    implicit none

    type :: date_t
        integer :: year
        integer :: month
        integer :: day
    end type

    type :: datetime_t
        type(date_t) :: date
    end type

    type(datetime_t) :: vals(9)
    integer :: ii

    ii = 3
    vals = [(datetime_t(date_t(2022, ii, 8)), ii = 1, 9)]

    print *, ii, vals(3)%date%month
end program implied_do_array_constructor_derived_type
