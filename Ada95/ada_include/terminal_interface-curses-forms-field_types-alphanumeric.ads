------------------------------------------------------------------------------
--                                                                          --
--                           GNAT ncurses Binding                           --
--                                                                          --
--          Terminal_Interface.Curses.Forms.Field_Types.AlphaNumeric        --
--                                                                          --
--                                 S P E C                                  --
--                                                                          --
--  Version 00.93                                                           --
--                                                                          --
--  The ncurses Ada95 binding is copyrighted 1996 by                        --
--  Juergen Pfeifer, Email: Juergen.Pfeifer@T-Online.de                     --
--                                                                          --
--  Permission is hereby granted to reproduce and distribute this           --
--  binding by any means and for any fee, whether alone or as part          --
--  of a larger distribution, in source or in binary form, PROVIDED         --
--  this notice is included with any such distribution, and is not          --
--  removed from any of its header files. Mention of ncurses and the        --
--  author of this binding in any applications linked with it is            --
--  highly appreciated.                                                     --
--                                                                          --
--  This binding comes AS IS with no warranty, implied or expressed.        --
------------------------------------------------------------------------------
--  Version Control:
--  $Revision: 1.2 $
------------------------------------------------------------------------------
package Terminal_Interface.Curses.Forms.Field_Types.AlphaNumeric is
   pragma Preelaborate (AlphaNumeric);

   type AlphaNumeric_Field is new Field_Type
     with record
        Minimum_Field_Width : Natural := 0;
     end record;

   procedure Set_Field_Type (Fld : in Field;
                             Typ : in AlphaNumeric_Field);

end Terminal_Interface.Curses.Forms.Field_Types.AlphaNumeric;
