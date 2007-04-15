program console;
 (*  
  * Console Demo usage for ownet/pascal module
  * 
  * Author:  George M. Zouganelis (gzoug@aueb.gr)
  * Version: $Id$
  *
  * This program is free software; you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation; either version 2 of the License, or (at
  * your option) any later version.
  *
  * This program is distributed in the hope that it will be useful, but
  * WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * General Public License for more details.
  *
  * You should have received a copy of the GNU General Public License
  * along with this program; if not, write to the Free Software
  * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
  *)


{$IFNDEF FPC}
  {$APPTYPE CONSOLE}
{$ELSE}
  {$mode objfpc}{$H+}
{$ENDIF}

uses
  Classes, SysUtils, ownet;

var
  o : TOWNet;
  t : TStringList;
  i : Integer;
begin

  o := TOWNet.Create('10.0.0.102:4304');
  t := o.Dir('/28.C072C0000000/');
  for i:=0 to t.Count-1 do  writeln(t.Strings[i]);


  writeln(o.Read('/28.C072C0000000/temperature'));
  writeln(o.Read('/uncached/28.C072C0000000/temperature11'));

  freeandnil(o);
  readln;
end.



