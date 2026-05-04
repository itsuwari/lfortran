program c_backend_print_array_record_01
    implicit none

    real(8) :: a(3), b(2, 2), c(2, 2, 2)

    a = [1.0d0, 2.0d0, 3.0d0]
    b = reshape([1.0d0, 2.0d0, 3.0d0, 4.0d0], [2, 2])
    c = reshape([1.0d0, 2.0d0, 3.0d0, 4.0d0, &
        5.0d0, 6.0d0, 7.0d0, 8.0d0], [2, 2, 2])

    print *, "ARR", a
    print *, "MAT", 7, b
    print *, "SEC", c(:, 2, :)
end program
