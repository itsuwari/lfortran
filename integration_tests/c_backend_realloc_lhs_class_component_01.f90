program c_backend_realloc_lhs_class_component_01
    implicit none

    type :: tagged_entry
        class(*), allocatable :: raw(:)
        integer, allocatable :: shape(:)
    end type

    type(tagged_entry) :: tmp, copy
    real(8) :: scalar
    real(8) :: values(2)

    scalar = 2.5d0
    values = [1.0d0, 3.0d0]

    tmp%raw = [scalar]
    allocate(tmp%shape(0))

    if (.not. allocated(tmp%raw)) error stop

    tmp%raw = values

    if (.not. allocated(tmp%raw)) error stop

    copy = tmp

    if (.not. allocated(copy%raw)) error stop
end program
