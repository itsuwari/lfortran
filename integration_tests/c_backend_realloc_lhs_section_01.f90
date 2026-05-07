program c_backend_realloc_lhs_section_01
    implicit none

    type :: row_type
        character(len=:), allocatable :: label
        integer :: value = 0
    end type

    character(len=5), allocatable :: strings(:)
    character(len=5), allocatable :: string_tmp(:)
    type(row_type), allocatable :: rows(:)
    type(row_type), allocatable :: row_tmp(:)

    allocate(strings(4), string_tmp(2))
    strings = ["aaaaa", "bbbbb", "ccccc", "ddddd"]
    string_tmp = ["left ", "right"]
    strings(:2) = string_tmp(:2)

    if (size(strings) /= 4) error stop
    if (strings(1) /= "left ") error stop
    if (strings(2) /= "right") error stop
    if (strings(3) /= "ccccc") error stop

    allocate(rows(3), row_tmp(2))
    rows(1)%label = "one"
    rows(1)%value = 1
    rows(2)%label = "two"
    rows(2)%value = 2
    rows(3)%label = "three"
    rows(3)%value = 3
    row_tmp(1)%label = "uno"
    row_tmp(1)%value = 10
    row_tmp(2)%label = "dos"
    row_tmp(2)%value = 20

    rows(:2) = row_tmp(:2)

    if (size(rows) /= 3) error stop
    if (rows(1)%label /= "uno" .or. rows(1)%value /= 10) error stop
    if (rows(2)%label /= "dos" .or. rows(2)%value /= 20) error stop
    if (rows(3)%label /= "three" .or. rows(3)%value /= 3) error stop
end program
