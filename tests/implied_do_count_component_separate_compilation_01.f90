program implied_do_count_component_separate_compilation_01
    type :: t
        integer :: first
    end type

    type(t), allocatable :: token(:)
    integer :: label(2), line(2), it

    allocate(token(2))
    token%first = [1, 4]
    label = [2, 5]
    line(:) = [(count(token%first <= label(it)), it = 1, size(label))]

    if (line(1) /= 1) error stop
    if (line(2) /= 2) error stop
end program implied_do_count_component_separate_compilation_01
