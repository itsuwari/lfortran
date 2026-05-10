      program c_backend_formatted_char_implied_do_01
      implicit none
      integer j, nout
      nout = 6
      if (.false.) then
         write(nout, 9993) 'orthogonal', '''',
     $        'transpose', ('''', j = 1, 4)
      endif
 9993 format(a, a, a, 4a)
      end
