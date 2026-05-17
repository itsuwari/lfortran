program c_backend_rank2_section_matmul_dynamic_01
implicit none

real(8) :: xyz(3, 4), abc(3, 4), lattice(3, 3), expected(3, 4)
integer :: i, j, k, ilt, iat

ilt = 2
iat = 3

do j = 1, 4
    do i = 1, 3
        xyz(i, j) = real(10 * j + i, 8)
        abc(i, j) = real(100 * j + i, 8)
    end do
end do

do j = 1, 3
    do i = 1, 3
        lattice(i, j) = real(10 * i + j, 8)
    end do
end do

expected = xyz
do j = 1, iat
    do i = 1, ilt
        do k = 1, ilt
            expected(i, j) = expected(i, j) + lattice(i, k) * abc(k, j)
        end do
    end do
end do

xyz(:ilt, :iat) = xyz(:ilt, :iat) + matmul(lattice(:ilt, :ilt), abc(:ilt, :iat))

do j = 1, 4
    do i = 1, 3
        if (xyz(i, j) - expected(i, j) > 1.0e-12_8) error stop
        if (expected(i, j) - xyz(i, j) > 1.0e-12_8) error stop
    end do
end do

end program
