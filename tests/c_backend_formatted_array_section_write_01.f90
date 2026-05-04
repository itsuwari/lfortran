program c_backend_formatted_array_section_write_01
    implicit none

    real(8), allocatable :: values(:, :)
    real(8), allocatable :: vector(:)

    allocate(values(3, 2))
    allocate(vector(3))
    values = reshape([1.0d0, 2.0d0, 3.0d0, 4.0d0, 5.0d0, 6.0d0], [3, 2])
    vector = [7.0d0, 8.0d0, 9.0d0]
    call check_section(values)
    call check_whole_array(vector)

contains

    subroutine check_section(a)
        real(8), intent(in) :: a(:, :)
        character(128) :: line

        write(line, '(i2, 1x, *(1x, f5.1))') 7, a(:, 2)

        if (index(line, '  4.0') == 0) error stop 'missing first section element'
        if (index(line, '  5.0') == 0) error stop 'missing second section element'
        if (index(line, '  6.0') == 0) error stop 'missing third section element'
    end subroutine

    subroutine check_whole_array(a)
        real(8), intent(in) :: a(:)
        character(4) :: label
        character(128) :: line

        label = 'vec'
        write(line, '(a4, 1x, *(1x, f5.1))') label, a

        if (index(line, '  7.0') == 0) error stop 'missing first array element'
        if (index(line, '  8.0') == 0) error stop 'missing second array element'
        if (index(line, '  9.0') == 0) error stop 'missing third array element'
    end subroutine

end program
