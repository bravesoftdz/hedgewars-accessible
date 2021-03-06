(*
 * Hedgewars, a free turn based strategy game
 * Copyright (c) 2004-2011 Andrey Korotaev <unC0Rr@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *)

{$INCLUDE "options.inc"}

unit uLandObjects;
interface
uses SDLh;

procedure AddObjects();
procedure FreeLandObjects();
procedure LoadThemeConfig;
procedure BlitImageAndGenerateCollisionInfo(cpX, cpY, Width: Longword; Image: PSDL_Surface);
procedure AddOnLandObjects(Surface: PSDL_Surface);

implementation
uses uStore, uConsts, uConsole, uRandom, uSound, GLunit,
     uTypes, uVariables, uUtils, uDebug, sysutils;

const MaxRects = 512;
      MAXOBJECTRECTS = 16;
      MAXTHEMEOBJECTS = 32;

type PRectArray = ^TRectsArray;
     TRectsArray = array[0..MaxRects] of TSDL_Rect;
     TThemeObject = record
                    Surf: PSDL_Surface;
                    inland: TSDL_Rect;
                    outland: array[0..Pred(MAXOBJECTRECTS)] of TSDL_Rect;
                    rectcnt: Longword;
                    Width, Height: Longword;
                    Maxcnt: Longword;
                    end;
     TThemeObjects = record
                     Count: LongInt;
                     objs: array[0..Pred(MAXTHEMEOBJECTS)] of TThemeObject;
                     end;
     TSprayObject = record
                    Surf: PSDL_Surface;
                    Width, Height: Longword;
                    Maxcnt: Longword;
                    end;
     TSprayObjects = record
                     Count: LongInt;
                     objs: array[0..Pred(MAXTHEMEOBJECTS)] of TSprayObject
                     end;

var Rects: PRectArray;
    RectCount: Longword;
    ThemeObjects: TThemeObjects;
    SprayObjects: TSprayObjects;


procedure BlitImageAndGenerateCollisionInfo(cpX, cpY, Width: Longword; Image: PSDL_Surface);
var p: PLongwordArray;
    x, y: Longword;
    bpp: LongInt;
begin
WriteToConsole('Generating collision info... ');

if SDL_MustLock(Image) then
   SDLTry(SDL_LockSurface(Image) >= 0, true);

bpp:= Image^.format^.BytesPerPixel;
TryDo(bpp = 4, 'Land object should be 32bit', true);

if Width = 0 then Width:= Image^.w;

p:= Image^.pixels;
for y:= 0 to Pred(Image^.h) do
    begin
    for x:= 0 to Pred(Width) do
        begin
            if (cReducedQuality and rqBlurryLand) = 0 then
            begin
                if LandPixels[cpY + y, cpX + x] = 0 then
                    LandPixels[cpY + y, cpX + x]:= p^[x];
            end
            else
                if LandPixels[(cpY + y) div 2, (cpX + x) div 2] = 0 then
                    LandPixels[(cpY + y) div 2, (cpX + x) div 2]:= p^[x];

        if ((Land[cpY + y, cpX + x] and $FF00) = 0) and ((p^[x] and AMask) <> 0) then
            Land[cpY + y, cpX + x]:= lfObject
        end;
    p:= @(p^[Image^.pitch shr 2])
    end;

if SDL_MustLock(Image) then
   SDL_UnlockSurface(Image);
WriteLnToConsole(msgOK)
end;

procedure AddRect(x1, y1, w1, h1: LongInt);
begin
with Rects^[RectCount] do
     begin
     x:= x1;
     y:= y1;
     w:= w1;
     h:= h1
     end;
inc(RectCount);
TryDo(RectCount < MaxRects, 'AddRect: overflow', true)
end;

procedure InitRects;
begin
RectCount:= 0;
New(Rects)
end;

procedure FreeRects;
begin
    Dispose(rects)
end;

function CheckIntersect(x1, y1, w1, h1: LongInt): boolean;
var i: Longword;
    res: boolean = false;
begin

i:= 0;
if RectCount > 0 then
   repeat
   with Rects^[i] do
        res:= (x < x1 + w1) and (x1 < x + w) and
                 (y < y1 + h1) and (y1 < y + h);
   inc(i)
   until (i = RectCount) or (res);
CheckIntersect:= res;
end;

function AddGirder(gX: LongInt): boolean;
var tmpsurf: PSDL_Surface;
    x1, x2, y, k, i: LongInt;
    rr: TSDL_Rect;
    bRes: boolean;

    function CountNonZeroz(x, y: LongInt): Longword;
    var i: LongInt;
        lRes: Longword;
    begin
    lRes:= 0;
    for i:= y to y + 15 do
        if Land[i, x] <> 0 then inc(lRes);
    CountNonZeroz:= lRes;
    end;

begin
y:= topY+150;
repeat
    inc(y, 24);
    x1:= gX;
    x2:= gX;

    while (x1 > Longint(leftX)+150) and (CountNonZeroz(x1, y) = 0) do dec(x1, 2);

    i:= x1 - 12;
    repeat
        dec(x1, 2);
        k:= CountNonZeroz(x1, y)
    until (x1 < Longint(leftX)+150) or (k = 0) or (k = 16) or (x1 < i);

    inc(x1, 2);
    if k = 16 then
        begin
        while (x2 < (rightX-150)) and (CountNonZeroz(x2, y) = 0) do inc(x2, 2);
        i:= x2 + 12;
        repeat
        inc(x2, 2);
        k:= CountNonZeroz(x2, y)
        until (x2 >= (rightX-150)) or (k = 0) or (k = 16) or (x2 > i) or (x2 - x1 >= 768);
        if (x2 < (rightX - 150)) and (k = 16) and (x2 - x1 > 250) and (x2 - x1 < 768)
            and not CheckIntersect(x1 - 32, y - 64, x2 - x1 + 64, 144) then break;
        end;
x1:= 0;
until y > (LAND_HEIGHT-125);

if x1 > 0 then
begin
    bRes:= true;
    tmpsurf:= LoadImage(Pathz[ptCurrTheme] + '/Girder', ifTransparent or ifIgnoreCaps);
    if tmpsurf = nil then tmpsurf:= LoadImage(Pathz[ptGraphics] + '/Girder', ifCritical or ifTransparent or ifIgnoreCaps);

    rr.x:= x1;
    while rr.x < x2 do
        begin
        BlitImageAndGenerateCollisionInfo(rr.x, y, min(x2 - rr.x, tmpsurf^.w), tmpsurf);
        inc(rr.x, tmpsurf^.w);
        end;
    SDL_FreeSurface(tmpsurf);

    AddRect(x1 - 8, y - 32, x2 - x1 + 16, 80);
end
else bRes:= false;

AddGirder:= bRes;
end;

function CheckLand(rect: TSDL_Rect; dX, dY, Color: Longword): boolean;
var tmpx, tmpx2, tmpy, tmpy2, bx, by: LongInt;
    bRes: boolean = true;
begin
inc(rect.x, dX);
inc(rect.y, dY);
bx:= rect.x + rect.w;
by:= rect.y + rect.h;
{$WARNINGS OFF}
tmpx:= rect.x;
tmpx2:= bx;
while (tmpx <= bx - rect.w div 2 - 1) and bRes do
      begin
      bRes:= ((rect.y and LAND_HEIGHT_MASK) = 0) and ((by and LAND_HEIGHT_MASK) = 0) and
             ((tmpx and LAND_WIDTH_MASK) = 0) and ((tmpx2 and LAND_WIDTH_MASK) = 0) and
             (Land[rect.y, tmpx] = Color) and (Land[by, tmpx] = Color) and
             (Land[rect.y, tmpx2] = Color) and (Land[by, tmpx2] = Color);
      inc(tmpx);
      dec(tmpx2)
      end;
tmpy:= rect.y+1;
tmpy2:= by-1;
while (tmpy <= by - rect.h div 2 - 1) and bRes do
      begin
      bRes:= ((tmpy and LAND_HEIGHT_MASK) = 0) and ((tmpy2 and LAND_HEIGHT_MASK) = 0) and
             ((rect.x and LAND_WIDTH_MASK) = 0) and ((bx and LAND_WIDTH_MASK) = 0) and
             (Land[tmpy, rect.x] = Color) and (Land[tmpy, bx] = Color) and
             (Land[tmpy2, rect.x] = Color) and (Land[tmpy2, bx] = Color);
      inc(tmpy);
      dec(tmpy2)
      end;
{$WARNINGS ON}
CheckLand:= bRes;
end;

function CheckCanPlace(x, y: Longword; var Obj: TThemeObject): boolean;
var i: Longword;
    bRes: boolean;
begin
with Obj do
     if CheckLand(inland, x, y, lfBasic) then
        begin
        bRes:= true;
        i:= 1;
        while bRes and (i <= rectcnt) do
              begin
              bRes:= CheckLand(outland[i], x, y, 0);
              inc(i)
              end;
        if bRes then
           bRes:= not CheckIntersect(x, y, Width, Height)
        end else
        bRes:= false;
CheckCanPlace:= bRes;
end;

function TryPut(var Obj: TThemeObject): boolean; overload;
const MaxPointsIndex = 2047;
var x, y: Longword;
    ar: array[0..MaxPointsIndex] of TPoint;
    cnt, i: Longword;
    bRes: boolean;
begin
cnt:= 0;
with Obj do
     begin
     if Maxcnt = 0 then
        exit(false);
     x:= 0;
     repeat
         y:= topY+32; // leave room for a hedgie to teleport in
         repeat
             if CheckCanPlace(x, y, Obj) then
                begin
                ar[cnt].x:= x;
                ar[cnt].y:= y;
                inc(cnt);
                if cnt > MaxPointsIndex then // buffer is full, do not check the rest land
                   begin
                   y:= 5000;
                   x:= 5000;
                   end
                end;
             inc(y, 3);
         until y >= LAND_HEIGHT - Height;
         inc(x, getrandom(6) + 3)
     until x >= LAND_WIDTH - Width;
     bRes:= cnt <> 0;
     if bRes then
        begin
        i:= getrandom(cnt);
        BlitImageAndGenerateCollisionInfo(ar[i].x, ar[i].y, 0, Obj.Surf);
        AddRect(ar[i].x, ar[i].y, Width, Height);
        dec(Maxcnt)
        end else Maxcnt:= 0
     end;
TryPut:= bRes;
end;

function TryPut(var Obj: TSprayObject; Surface: PSDL_Surface): boolean; overload;
const MaxPointsIndex = 8095;
var x, y: Longword;
    ar: array[0..MaxPointsIndex] of TPoint;
    cnt, i: Longword;
    r: TSDL_Rect;
    bRes: boolean;
begin
cnt:= 0;
with Obj do
    begin
    if Maxcnt = 0 then
        exit(false);
    x:= 0;
    r.x:= 0;
    r.y:= 0;
    r.w:= Width;
    r.h:= Height + 16;
    repeat
        y:= 8;
        repeat
            if CheckLand(r, x, y - 8, lfBasic)
            and not CheckIntersect(x, y, Width, Height) then
            begin
            ar[cnt].x:= x;
            ar[cnt].y:= y;
            inc(cnt);
            if cnt > MaxPointsIndex then // buffer is full, do not check the rest land
                begin
                y:= 5000;
                x:= 5000;
                end
            end;
            inc(y, 12);
        until y >= LAND_HEIGHT - Height - 8;
        inc(x, getrandom(12) + 12)
    until x >= LAND_WIDTH - Width;
    bRes:= cnt <> 0;
    if bRes then
        begin
        i:= getrandom(cnt);
        r.x:= ar[i].X;
        r.y:= ar[i].Y;
        r.w:= Width;
        r.h:= Height;
        SDL_UpperBlit(Obj.Surf, nil, Surface, @r);
        AddRect(ar[i].x - 32, ar[i].y - 32, Width + 64, Height + 64);
        dec(Maxcnt)
        end else Maxcnt:= 0
    end;
TryPut:= bRes;
end;

procedure ReadThemeInfo(var ThemeObjects: TThemeObjects; var SprayObjects: TSprayObjects);
var s, key: shortstring;
    f: textfile;
    i, ii, numFlakes: LongInt;
    c1, c2: TSDL_Color;

    procedure CheckRect(Width, Height, x, y, w, h: LongWord);
    begin
    if (x + w > Width) then OutError('Object''s rectangle exceeds image: x + w (' + inttostr(x) + ' + ' + inttostr(w) + ') > Width (' + inttostr(Width) + ')', true);
    if (y + h > Height) then OutError('Object''s rectangle exceeds image: y + h (' + inttostr(y) + ' + ' + inttostr(h) + ') > Height (' + inttostr(Height) + ')', true);
    end;

begin

AddProgress;

s:= Pathz[ptCurrTheme] + '/' + cThemeCFGFilename;
WriteLnToConsole('Reading objects info...');
Assign(f, s);
{$I-}
filemode:= 0; // readonly
Reset(f);

ThemeObjects.Count:= 0;
SprayObjects.Count:= 0;

while not eof(f) do
    begin
    Readln(f, s);
    if Length(s) = 0 then continue;
    if s[1] = ';' then continue;

    i:= Pos('=', s);
    key:= Trim(Copy(s, 1, Pred(i)));
    Delete(s, 1, i);

    if key = 'sky' then
        begin
        i:= Pos(',', s);
        c1.r:= StrToInt(Trim(Copy(s, 1, Pred(i))));
        Delete(s, 1, i);
        i:= Pos(',', s);
        c1.g:= StrToInt(Trim(Copy(s, 1, Pred(i))));
        Delete(s, 1, i);
        c1.b:= StrToInt(Trim(s));
        glClearColor(c1.r / 255, c1.g / 255, c1.b / 255, 0.99);
        SDSkyColor.r:= byte(c1.r * SDTint div 255);
        SDSkyColor.g:= byte(c1.g * SDTint div 255);
        SDSkyColor.b:= byte(c1.b * SDTint div 255);
        end
    else if key = 'border' then
        begin
        i:= Pos(',', s);
        c2.r:= StrToInt(Trim(Copy(s, 1, Pred(i))));
        Delete(s, 1, i);
        i:= Pos(',', s);
        c2.g:= StrToInt(Trim(Copy(s, 1, Pred(i))));
        Delete(s, 1, i);
        c2.b:= StrToInt(Trim(s));
        cExplosionBorderColor:= c2.value or AMask;
        end
    else if key = 'water-top' then
        begin
        i:= Pos(',', s);
        WaterColorArray[0].r:= StrToInt(Trim(Copy(s, 1, Pred(i))));
        Delete(s, 1, i);
        i:= Pos(',', s);
        WaterColorArray[0].g:= StrToInt(Trim(Copy(s, 1, Pred(i))));
        Delete(s, 1, i);
        WaterColorArray[0].b:= StrToInt(Trim(s));
        WaterColorArray[0].a := 255;
        WaterColorArray[1]:= WaterColorArray[0];
        end
    else if key = 'water-bottom' then
        begin
        i:= Pos(',', s);
        WaterColorArray[2].r:= StrToInt(Trim(Copy(s, 1, Pred(i))));
        Delete(s, 1, i);
        i:= Pos(',', s);
        WaterColorArray[2].g:= StrToInt(Trim(Copy(s, 1, Pred(i))));
        Delete(s, 1, i);
        WaterColorArray[2].b:= StrToInt(Trim(s));
        WaterColorArray[2].a := 255;
        WaterColorArray[3]:= WaterColorArray[2];
        end
    else if key = 'water-opacity' then
        begin
        cWaterOpacity:= StrToInt(Trim(s));
        cSDWaterOpacity:= cWaterOpacity
        end
    else if key = 'music' then MusicFN:= Trim(s)
    else if key = 'clouds' then
        begin
        cCloudsNumber:= StrToInt(Trim(s)) * cScreenSpace div LAND_WIDTH;
        cSDCloudsNumber:= cCloudsNumber
        end
    else if key = 'object' then
        begin
        inc(ThemeObjects.Count);
        with ThemeObjects.objs[Pred(ThemeObjects.Count)] do
            begin
            i:= Pos(',', s);
            Surf:= LoadImage(Pathz[ptCurrTheme] + '/' + Trim(Copy(s, 1, Pred(i))), ifCritical or ifTransparent or ifIgnoreCaps);
            Width:= Surf^.w;
            Height:= Surf^.h;
            Delete(s, 1, i);
            i:= Pos(',', s);
            Maxcnt:= StrToInt(Trim(Copy(s, 1, Pred(i))));
            Delete(s, 1, i);
            if (Maxcnt < 1) or (Maxcnt > MAXTHEMEOBJECTS) then OutError('Object''s max count should be between 1 and '+ inttostr(MAXTHEMEOBJECTS) +' (it was '+ inttostr(Maxcnt) +').', true);
            with inland do
                begin
                i:= Pos(',', s);
                x:= StrToInt(Trim(Copy(s, 1, Pred(i))));
                Delete(s, 1, i);
                i:= Pos(',', s);
                y:= StrToInt(Trim(Copy(s, 1, Pred(i))));
                Delete(s, 1, i);
                i:= Pos(',', s);
                w:= StrToInt(Trim(Copy(s, 1, Pred(i))));
                Delete(s, 1, i);
                i:= Pos(',', s);
                h:= StrToInt(Trim(Copy(s, 1, Pred(i))));
                Delete(s, 1, i);
                CheckRect(Width, Height, x, y, w, h)
                end;
            i:= Pos(',', s);
            rectcnt:= StrToInt(Trim(Copy(s, 1, Pred(i))));
            Delete(s, 1, i);
            for ii:= 1 to rectcnt do
                with outland[ii] do
                    begin
                    i:= Pos(',', s);
                    x:= StrToInt(Trim(Copy(s, 1, Pred(i))));
                    Delete(s, 1, i);
                    i:= Pos(',', s);
                    y:= StrToInt(Trim(Copy(s, 1, Pred(i))));
                    Delete(s, 1, i);
                    i:= Pos(',', s);
                    w:= StrToInt(Trim(Copy(s, 1, Pred(i))));
                    Delete(s, 1, i);
                    if ii = rectcnt then h:= StrToInt(Trim(s))
                    else
                        begin
                        i:= Pos(',', s);
                        h:= StrToInt(Trim(Copy(s, 1, Pred(i))));
                        Delete(s, 1, i)
                        end;
                    CheckRect(Width, Height, x, y, w, h)
                    end;
            end;
        end
    else if key = 'spray' then
        begin
        inc(SprayObjects.Count);
        with SprayObjects.objs[Pred(SprayObjects.Count)] do
            begin
            i:= Pos(',', s);
            Surf:= LoadImage(Pathz[ptCurrTheme] + '/' + Trim(Copy(s, 1, Pred(i))), ifCritical or ifTransparent or ifIgnoreCaps);
            Width:= Surf^.w;
            Height:= Surf^.h;
            Delete(s, 1, i);
            Maxcnt:= StrToInt(Trim(s));
            end;
        end
    else if key = 'flakes' then
        begin
        i:= Pos(',', s);
        vobCount:= StrToInt(Trim(Copy(s, 1, Pred(i))));
        Delete(s, 1, i);
        if vobCount > 0 then
            begin
            i:= Pos(',', s);
            vobFramesCount:= StrToInt(Trim(Copy(s, 1, Pred(i))));
            Delete(s, 1, i);
            i:= Pos(',', s);
            vobFrameTicks:= StrToInt(Trim(Copy(s, 1, Pred(i))));
            Delete(s, 1, i);
            i:= Pos(',', s);
            vobVelocity:= StrToInt(Trim(Copy(s, 1, Pred(i))));
            Delete(s, 1, i);
            vobFallSpeed:= StrToInt(Trim(s));
            end;
        end
    else if key = 'sd-water-top' then
        begin
        i:= Pos(',', s);
        SDWaterColorArray[0].r:= StrToInt(Trim(Copy(s, 1, Pred(i))));
        Delete(s, 1, i);
        i:= Pos(',', s);
        SDWaterColorArray[0].g:= StrToInt(Trim(Copy(s, 1, Pred(i))));
        Delete(s, 1, i);
        SDWaterColorArray[0].b:= StrToInt(Trim(s));
        SDWaterColorArray[0].a := 255;
        SDWaterColorArray[1]:= SDWaterColorArray[0];
        end
    else if key = 'sd-water-bottom' then
        begin
        i:= Pos(',', s);
        SDWaterColorArray[2].r:= StrToInt(Trim(Copy(s, 1, Pred(i))));
        Delete(s, 1, i);
        i:= Pos(',', s);
        SDWaterColorArray[2].g:= StrToInt(Trim(Copy(s, 1, Pred(i))));
        Delete(s, 1, i);
        SDWaterColorArray[2].b:= StrToInt(Trim(s));
        SDWaterColorArray[2].a := 255;
        SDWaterColorArray[3]:= SDWaterColorArray[2];
        end
    else if key = 'sd-water-opacity' then cSDWaterOpacity:= StrToInt(Trim(s))
    else if key = 'sd-clouds' then cSDCloudsNumber:= StrToInt(Trim(s)) * cScreenSpace div LAND_WIDTH
    else if key = 'sd-flakes' then
        begin
        i:= Pos(',', s);
        vobSDCount:= StrToInt(Trim(Copy(s, 1, Pred(i))));
        Delete(s, 1, i);
        if vobSDCount > 0 then
            begin
            i:= Pos(',', s);
            vobSDFramesCount:= StrToInt(Trim(Copy(s, 1, Pred(i))));
            Delete(s, 1, i);
            i:= Pos(',', s);
            vobSDFrameTicks:= StrToInt(Trim(Copy(s, 1, Pred(i))));
            Delete(s, 1, i);
            i:= Pos(',', s);
            vobSDVelocity:= StrToInt(Trim(Copy(s, 1, Pred(i))));
            Delete(s, 1, i);
            vobSDFallSpeed:= StrToInt(Trim(s));
            end;
        end
    else if key = 'rq-sky' then
        begin
        if ((cReducedQuality and rqNoBackground) <> 0) then
            begin
            i:= Pos(',', s);
            c1.r:= StrToInt(Trim(Copy(s, 1, Pred(i))));
            Delete(s, 1, i);
            i:= Pos(',', s);
            c1.g:= StrToInt(Trim(Copy(s, 1, Pred(i))));
            Delete(s, 1, i);
            c1.b:= StrToInt(Trim(s));
            glClearColor(c1.r / 255, c1.g / 255, c1.b / 255, 0.99);
            SDSkyColor.r:= byte(c1.r * SDTint div 255);
            SDSkyColor.g:= byte(c1.g * SDTint div 255);
            SDSkyColor.b:= byte(c1.b * SDTint div 255);
            end
        end
    end;

Close(f);
{$I+}
TryDo(IOResult = 0, 'Bad data or cannot access file ' + cThemeCFGFilename, true);
AddProgress;
end;

procedure AddThemeObjects(var ThemeObjects: TThemeObjects);
var i, ii, t: LongInt;
    b: boolean;
begin
    if ThemeObjects.Count = 0 then exit;
    WriteLnToConsole('Adding theme objects...');

    for i:=0 to ThemeObjects.Count do
        ThemeObjects.objs[i].Maxcnt := max(1, (ThemeObjects.objs[i].Maxcnt * MaxHedgehogs) div 18); // Maxcnt is proportional to map size, but allow objects to span even if we're on a tiny map

    repeat
        t := getrandom(ThemeObjects.Count);
        b := false;
        for i:=0 to ThemeObjects.Count do
            begin
            ii := (i+t) mod ThemeObjects.Count;

            if ThemeObjects.objs[ii].Maxcnt <> 0 then
                b := b or TryPut(ThemeObjects.objs[ii])
            end;
    until not b;
end;

procedure AddSprayObjects(Surface: PSDL_Surface; var SprayObjects: TSprayObjects);
var i, ii, t: LongInt;
    b: boolean;
begin
    if SprayObjects.Count = 0 then exit;
    WriteLnToConsole('Adding spray objects...');

    for i:=0 to SprayObjects.Count do
        SprayObjects.objs[i].Maxcnt := max(1, (SprayObjects.objs[i].Maxcnt * MaxHedgehogs) div 18); // Maxcnt is proportional to map size, but allow objects to span even if we're on a tiny map

    repeat
        t := getrandom(SprayObjects.Count);
        b := false;
        for i:=0 to SprayObjects.Count do
            begin
            ii := (i+t) mod SprayObjects.Count;

            if SprayObjects.objs[ii].Maxcnt <> 0 then
                b := b or TryPut(SprayObjects.objs[ii], Surface)
            end;
    until not b;
end;

procedure AddObjects();
var i, int: Longword;
begin
InitRects;
if hasGirders then
    begin
    int:= max(playWidth div 8, 256);
    i:=leftX+int;
    repeat
        AddGirder(i);
        i:=i+int;
    until (i>rightX-int);
    end;
if (GameFlags and gfDisableLandObjects) = 0 then AddThemeObjects(ThemeObjects);
AddProgress();
FreeRects();
end;

procedure AddOnLandObjects(Surface: PSDL_Surface);
begin
InitRects;
//AddSprayObjects(Surface, SprayObjects, 12);
AddSprayObjects(Surface, SprayObjects);
FreeRects
end;

procedure LoadThemeConfig;
begin
    ReadThemeInfo(ThemeObjects, SprayObjects)
end;

procedure FreeLandObjects();
var i: Longword;
begin
    for i:= 0 to Pred(MAXTHEMEOBJECTS) do
    begin
        if ThemeObjects.objs[i].Surf <> nil then
            SDL_FreeSurface(ThemeObjects.objs[i].Surf);
        if SprayObjects.objs[i].Surf <> nil then
            SDL_FreeSurface(SprayObjects.objs[i].Surf);
        ThemeObjects.objs[i].Surf:= nil;
        SprayObjects.objs[i].Surf:= nil;
    end;
end;

end.
