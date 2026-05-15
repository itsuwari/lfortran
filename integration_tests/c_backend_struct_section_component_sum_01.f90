program c_backend_struct_section_component_sum_01
    implicit none

    type :: atom_data
        integer :: charge
        integer :: marker
    end type

    type(atom_data), allocatable :: atoms(:)
    integer :: total

    allocate(atoms(4))
    atoms(1)%charge = 2
    atoms(2)%charge = -1
    atoms(3)%charge = 3
    atoms(4)%charge = 100
    atoms(:)%marker = 42

    total = sum(atoms(:3)%charge)

    if (total /= 4) error stop "wrong section component sum"
    if (any(atoms(:3)%marker /= 42)) error stop "section component corrupted"
end program c_backend_struct_section_component_sum_01
