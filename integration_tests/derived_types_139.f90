program derived_types_139
    implicit none

    type :: item
        integer :: value
    end type

    type :: dictionary
        type(item), allocatable :: record(:)
    end type

    type(dictionary) :: dict
    class(*), allocatable :: h

    dict = dictionary(record=null())

    if (allocated(dict%record)) error stop
    if (allocated(h)) error stop
end program derived_types_139
