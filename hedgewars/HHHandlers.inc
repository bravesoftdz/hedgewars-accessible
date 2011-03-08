(*
 * Hedgewars, a free turn based strategy game
 * Copyright (c) 2004-2010 Andrey Korotaev <unC0Rr@gmail.com>
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

////////////////////////////////////////////////////////////////////////////////

procedure HHHurt(Hedgehog: PHedgehog; Source: TDamageSource);
begin
if (Source = dsFall) or (Source = dsExplosion) then
    case random(3) of
        0: PlaySound(sndOoff1, Hedgehog^.Team^.voicepack);
        1: PlaySound(sndOoff2, Hedgehog^.Team^.voicepack);
        2: PlaySound(sndOoff3, Hedgehog^.Team^.voicepack);
    end
else if (Source = dsPoison) then
    case random(2) of
        0: PlaySound(sndPoisonCough, Hedgehog^.Team^.voicepack);
        1: PlaySound(sndPoisonMoan, Hedgehog^.Team^.voicepack);
    end
else
    case random(4) of
        0: PlaySound(sndOw1, Hedgehog^.Team^.voicepack);
        1: PlaySound(sndOw2, Hedgehog^.Team^.voicepack);
        2: PlaySound(sndOw3, Hedgehog^.Team^.voicepack);
        3: PlaySound(sndOw4, Hedgehog^.Team^.voicepack);
    end
end;

// Shouldn't more of this ammo switching stuff be moved to uAmmos ?
function ChangeAmmo(Gear: PGear): boolean;
var slot, i: Longword;
    ammoidx: LongInt;
begin
ChangeAmmo:= false;
slot:= Gear^.MsgParam;

with Gear^.Hedgehog^ do
    begin
    Gear^.Message:= Gear^.Message and not gmSlot;
    ammoidx:= 0;
    if ((Gear^.State and (gstAttacking or gstAttacked)) <> 0) or
       (TargetPoint.X <> NoPointX) or
       ((MultiShootAttacks > 0) and ((Ammoz[CurAmmoType].Ammo.Propz and ammoprop_NoRoundEnd) = 0)) or
       ((Gear^.State and gstHHDriven) = 0) then exit;
    ChangeAmmo:= true;

    while (ammoidx < cMaxSlotAmmoIndex) and (Ammo^[slot, ammoidx].AmmoType <> CurAmmoType) do inc(ammoidx);

    if ((Ammoz[CurAmmoType].Ammo.Propz and ammoprop_NoRoundEnd) <> 0) and (MultiShootAttacks > 0) then OnUsedAmmo(Gear^.Hedgehog^);

    MultiShootAttacks:= 0;
    Gear^.Message:= Gear^.Message and not (gmLJump or gmHJump);
    
    if Ammoz[CurAmmoType].Slot = slot then
        begin
        i:= 0;
        repeat
        inc(ammoidx);
        if (ammoidx > cMaxSlotAmmoIndex) then
            begin
            inc(i);
            CurAmmoType:= amNothing;
            ammoidx:= -1;
            //TryDo(i < 2, 'Engine bug: no ammo in current slot', true)
            end;
        until ((Ammo^[slot, ammoidx].Count > 0) and (Team^.Clan^.TurnNumber > Ammoz[Ammo^[slot, ammoidx].AmmoType].SkipTurns)) or (i = 1)
        end 
    else
        begin
        i:= 0;
        // check whether there is ammo in slot
        while (i <= cMaxSlotAmmoIndex)
          and ((Ammo^[slot, i].Count = 0)
               or (Team^.Clan^.TurnNumber <= Ammoz[Ammo^[slot, i].AmmoType].SkipTurns)) do inc(i);

        if i <= cMaxSlotAmmoIndex then ammoidx:= i
        else ammoidx:= -1
        end;
        if ammoidx >= 0 then CurAmmoType:= Ammo^[slot, ammoidx].AmmoType;
    end
end;

procedure HHSetWeapon(Gear: PGear);
var t: LongInt;
    weap: TAmmoType;
    Hedgehog: PHedgehog;
    s: boolean;
begin
weap:= TAmmoType(Gear^.MsgParam);
Hedgehog:= Gear^.Hedgehog;

if Hedgehog^.Team^.Clan^.TurnNumber <= Ammoz[weap].SkipTurns then exit; // weapon is not activated yet

Gear^.MsgParam:= Ammoz[weap].Slot;

t:= cMaxSlotAmmoIndex;

Gear^.Message:= Gear^.Message and not gmWeapon;

with Hedgehog^ do
    while (CurAmmoType <> weap) and (t >= 0) do
        begin
        s:= ChangeAmmo(Gear);
        dec(t)
        end;

if s then ApplyAmmoChanges(Gear^.Hedgehog^)
end;

procedure HHSetTimer(Gear: PGear);
var CurWeapon: PAmmo;
begin
Gear^.Message:= Gear^.Message and not gmTimer;
CurWeapon:= GetAmmoEntry(Gear^.Hedgehog^);
with Gear^.Hedgehog^ do
    if (CurWeapon^.Propz and ammoprop_Timerable) <> 0 then
        begin
        CurWeapon^.Timer:= 1000 * Gear^.MsgParam;
        with CurrentTeam^ do
            ApplyAmmoChanges(Hedgehogs[CurrHedgehog]);
        end;
end;


procedure Attack(Gear: PGear);
var xx, yy, newDx, newDy, lx, ly: hwFloat;
    tmpGear: PVisualGear;
    CurWeapon: PAmmo;
    altUse: boolean;
begin
bShowFinger:= false;
CurWeapon:= GetAmmoEntry(Gear^.Hedgehog^);
with Gear^,
     Gear^.Hedgehog^ do
     begin
     if ((State and gstHHDriven) <> 0)and
        ((State and (gstAttacked or gstHHChooseTarget)) = 0) and
        (((State and gstMoving) = 0) or
            // Allow attacks while moving on ammo with AltAttack
            ((CurAmmoGear <> nil) and ((Ammoz[CurAmmoGear^.AmmoType].Ammo.Propz and ammoprop_AltAttack) <> 0)) or
            ((Ammoz[CurAmmoType].Ammo.Propz and ammoprop_AttackInMove) <> 0)) and
        ((TargetPoint.X <> NoPointX) or ((Ammoz[CurAmmoType].Ammo.Propz and ammoprop_NeedTarget) = 0)) then
        begin
        State:= State or gstAttacking;
        if Power = cMaxPower then Message:= Message and not gmAttack
        else if (Ammoz[CurAmmoType].Ammo.Propz and ammoprop_Power) = 0 then Message:= Message and not gmAttack
        else begin
             if Power = 0 then
                begin
                AttackBar:= CurrentTeam^.AttackBar;
                PlaySound(sndThrowPowerUp)
                end;
             inc(Power)
             end;
        if ((Message and gmAttack) <> 0) then exit;

        if (Ammoz[CurAmmoType].Ammo.Propz and ammoprop_Power) <> 0 then
           begin
           StopSound(sndThrowPowerUp);
           PlaySound(sndThrowRelease);
           end;

        xx:= SignAs(AngleSin(Angle), dX);
        yy:= -AngleCos(Angle);

        lx:= X + int2hwfloat(round(GetLaunchX(CurAmmoType, hwSign(dX), Angle)));
        ly:= Y + int2hwfloat(round(GetLaunchY(CurAmmoType, Angle)));

        if ((Gear^.State and gstHHHJump) <> 0) and (not cArtillery) then xx:= - xx;
        if Ammoz[CurAmmoType].Ammo.AttackVoice <> sndNone then
           PlaySound(Ammoz[CurAmmoType].Ammo.AttackVoice, CurrentTeam^.voicepack);

// Initiating alt attack
        if  (CurAmmoGear <> nil) and
            ((Ammoz[CurAmmoGear^.AmmoType].Ammo.Propz and ammoprop_AltAttack) <> 0) and
            ((Gear^.Message and gmLJump) <> 0) and
            ((Ammoz[CurAmmoType].Ammo.Propz and ammoprop_AltUse) <> 0) then
            begin
            newDx:= dX / _2; 
            newDy:= dY / _2;
            altUse:= true;
            end
        else
            begin
            newDx:= xx*Power/cPowerDivisor;
            newDy:= yy*Power/cPowerDivisor;
            altUse:= false
            end;

             case CurAmmoType of
                      amGrenade: FollowGear:= AddGear(hwRound(lx), hwRound(ly), gtBomb,         0, newDx, newDy, CurWeapon^.Timer);
                      amMolotov: FollowGear:= AddGear(hwRound(lx), hwRound(ly), gtMolotov,      0, newDx, newDy, 0);
                  amClusterBomb: FollowGear:= AddGear(hwRound(lx), hwRound(ly), gtClusterBomb,  0, newDx, newDy, CurWeapon^.Timer);
                      amGasBomb: FollowGear:= AddGear(hwRound(lx), hwRound(ly), gtGasBomb,      0, newDx, newDy, CurWeapon^.Timer);
                      amBazooka: FollowGear:= AddGear(hwRound(lx), hwRound(ly), gtShell,        0, newDx, newDy, 0);
                     amSnowball: FollowGear:= AddGear(hwRound(lx), hwRound(ly), gtSnowball,     0, newDx, newDy, 0);
                          amBee: FollowGear:= AddGear(hwRound(lx), hwRound(ly), gtBee,          0, newDx, newDy, 0);
                      amShotgun: begin
                                 PlaySound(sndShotgunReload);
                                 CurAmmoGear:= AddGear(hwRound(lx), hwRound(ly), gtShotgunShot,  0, xx * _0_5, yy * _0_5, 0);
                                 end;
                   amPickHammer: CurAmmoGear:= AddGear(hwRound(lx), hwRound(ly) + cHHRadius, gtPickHammer, 0, _0, _0, 0);
                         amSkip: ParseCommand('/skip', true);
                         amRope: CurAmmoGear:= AddGear(hwRound(lx), hwRound(ly), gtRope, 0, xx, yy, 0);
                         amMine: if altUse then
                                    AddGear(hwRound(lx) + hwSign(dX) * 7, hwRound(ly), gtMine, gstWait, newDx, newDy, 3000)
                                 else
                                    AddGear(hwRound(lx) + hwSign(dX) * 7, hwRound(ly), gtMine, gstWait, SignAs(_0_02, dX), _0, 3000);
                        amSMine: FollowGear:= AddGear(hwRound(lx), hwRound(ly), gtSMine,    0, xx*Power/cPowerDivisor, yy*Power/cPowerDivisor, 0);
                       amDEagle: CurAmmoGear:= AddGear(hwRound(lx + xx * cHHRadius), hwRound(ly + yy * cHHRadius), gtDEagleShot, 0, xx * _0_5, yy * _0_5, 0);
                      amSineGun: CurAmmoGear:= AddGear(hwRound(lx + xx * cHHRadius), hwRound(ly + yy * cHHRadius), gtSineGunShot, 0, xx * _0_5, yy * _0_5, 0);
                    amPortalGun: AddGear(hwRound(lx + xx * cHHRadius), hwRound(ly + yy * cHHRadius), gtPortal, 0, xx * _0_6, yy * _0_6, 0);
                  amSniperRifle: begin
                                 PlaySound(sndSniperReload);
                                 CurAmmoGear:= AddGear(hwRound(lx + xx * cHHRadius), hwRound(ly + yy * cHHRadius), gtSniperRifleShot, 0, xx * _0_5, yy * _0_5, 0);
                                 end;
                     amDynamite: AddGear(hwRound(lx) + hwSign(dX) * 7, hwRound(ly), gtDynamite, 0, SignAs(_0_03, dX), _0, 5000);
                    amFirePunch: CurAmmoGear:= AddGear(hwRound(lx) + hwSign(dX) * 10, hwRound(ly), gtFirePunch, 0, xx, _0, 0);
                         amWhip: begin
                                 CurAmmoGear:= AddGear(hwRound(lx) + hwSign(dX) * 10, hwRound(ly), gtWhip, 0, SignAs(_1, dX), - _0_8, 0);
                                 PlaySound(sndWhipCrack)
                                 end;
                       amHammer: begin
                                 CurAmmoGear:= AddGear(hwRound(lx) + hwSign(dX) * 10, hwRound(ly), gtHammer, 0, SignAs(_1, dX), - _0_8, 0);
                                 PlaySound(sndWhack)
                                 end;
                  amBaseballBat: begin
                                 CurAmmoGear:= AddGear(hwRound(lx) + hwSign(dX) * 10, hwRound(ly), gtShover, gsttmpFlag, xx * _0_5, yy * _0_5, 0);
                                 PlaySound(sndBaseballBat) // TODO: Only play if something is hit?
                                 end;
                    amParachute: begin
                                 CurAmmoGear:= AddGear(hwRound(lx), hwRound(ly), gtParachute, 0, _0, _0, 0);
                                 PlaySound(sndParachute)
                                 end;
                    // we save CurWeapon^.Pos (in this case: cursor direction) by using it as (otherwise irrelevant) X value of the new gear.
                    amAirAttack: AddGear(CurWeapon^.Pos, 0, gtAirAttack, 0, _0, _0, 0);
                   amMineStrike: AddGear(CurWeapon^.Pos, 0, gtAirAttack, 1, _0, _0, 0);
                    amBlowTorch: CurAmmoGear:= AddGear(hwRound(lx), hwRound(ly), gtBlowTorch, 0, SignAs(_0_5, dX), _0, 0);
                       amGirder: CurAmmoGear:= AddGear(0, 0, gtGirder, CurWeapon^.Pos, _0, _0, 0);
                     amTeleport: CurAmmoGear:= AddGear(CurWeapon^.Pos, 0, gtTeleport, 0, _0, _0, 0);
                       amSwitch: CurAmmoGear:= AddGear(hwRound(lx), hwRound(ly), gtSwitcher, 0, _0, _0, 0);
                       amMortar: begin
                                 playSound(sndMortar);
                                 FollowGear:= AddGear(hwRound(lx), hwRound(ly), gtMortar,  0, xx*cMaxPower/cPowerDivisor, yy*cMaxPower/cPowerDivisor, 0);
                                 end;
                      amRCPlane: begin
                                 CurAmmoGear:= AddGear(hwRound(lx), hwRound(ly), gtRCPlane,  0, xx * cMaxPower / cPowerDivisor / 4, yy * cMaxPower / cPowerDivisor / 4, 0);
                                 CurAmmoGear^.SoundChannel:= LoopSound(sndRCPlane, nil)
                                 end;
                       amKamikaze: CurAmmoGear:= AddGear(hwRound(lx), hwRound(ly), gtKamikaze, 0, xx * _0_5, yy * _0_5, 0);
                         amCake: CurAmmoGear:= AddGear(hwRound(lx) + hwSign(dX) * 3, hwRound(ly), gtCake, 0, xx, _0, 0);
                    amSeduction: CurAmmoGear:= AddGear(hwRound(lx + xx * cHHRadius * 2), hwRound(ly + yy * cHHRadius * 2), gtSeduction, 0, xx * _0_4, yy * _0_4, 0);
                   amWatermelon: FollowGear:= AddGear(hwRound(lx), hwRound(ly), gtWatermelon,  0, newDx, newDy, CurWeapon^.Timer);
                  amHellishBomb: FollowGear:= AddGear(hwRound(lx), hwRound(ly), gtHellishBomb,    0, newDx, newDy, 0);
                       amNapalm: AddGear(CurWeapon^.Pos, 0, gtAirAttack, 2, _0, _0, 0);
                        amDrill: FollowGear:= AddGear(hwRound(lx), hwRound(ly), gtDrill, 0, newDx, newDy, 0);
                      amBallgun: CurAmmoGear:= AddGear(hwRound(X), hwRound(Y), gtBallgun,  0, xx * _0_5, yy * _0_5, 0);
                    amJetpack: CurAmmoGear:= AddGear(hwRound(lx), hwRound(ly), gtJetpack, 0, _0, _0, 0);
                    amBirdy: begin
                             PlaySound(sndWhistle);
                             CurAmmoGear:= AddGear(hwRound(lx), hwRound(ly) - 32, gtBirdy, 0, _0, _0, 0);
                             end;
                      amLowGravity: begin
                                    PlaySound(sndLowGravity);
                                    cGravity:= cMaxWindSpeed;
                                    cGravityf:= 0.00025
                                    end;
                      amExtraDamage:begin 
                                    PlaySound(sndHellishImpact4);
                                    cDamageModifier:= _1_5
                                    end;
                      amInvulnerable: Invulnerable:= true;
                      amExtraTime:  begin
                                    PlaySound(sndSwitchHog);
                                    TurnTimeLeft:= TurnTimeLeft + 30000
                                    end;
                      amLaserSight: cLaserSighting:= true;
                      amVampiric: begin
                                  PlaySound(sndOw1, Team^.voicepack);
                                  cVampiric:= true;
                                  end;
                      amPiano: begin
                               // Tuck the hedgehog away until the piano attack is completed
                               Unplaced:= true;
                               X:= _0;
                               Y:= _0;
                               FollowGear:= AddGear(TargetPoint.X, 0, gtPiano, 0, _0, _0, 0);
                               PauseMusic
                               end;
                      amFlamethrower: CurAmmoGear:= AddGear(hwRound(X), hwRound(Y), gtFlamethrower,  0, xx * _0_5, yy * _0_5, 0);
                    amResurrector: begin
                        CurAmmoGear:= AddGear(hwRound(lx), hwRound(ly),
                                gtResurrector, 0, _0, _0, 0);
                        CurAmmoGear^.SoundChannel := LoopSound(sndResurrector);
                    end;
                   amDrillStrike: AddGear(CurWeapon^.Pos, 0, gtAirAttack, 3, _0, _0, 0);
                   //amMelonStrike: AddGear(CurWeapon^.Pos, 0, gtAirAttack, 4, _0, _0, 0);
                  end;

        uStats.AmmoUsed(CurAmmoType);

        if not (SpeechText = '') then
            begin
            tmpGear:= AddVisualGear(0, 0, vgtSpeechBubble);
            if tmpGear <> nil then
               begin
               tmpGear^.Text:= SpeechText;
               tmpGear^.Hedgehog:= Gear^.Hedgehog;
               tmpGear^.FrameTicks:= SpeechType;
               end;
            SpeechText:= ''
            end;

        Power:= 0;
        if (CurAmmoGear <> nil)
           and ((Ammoz[CurAmmoType].Ammo.Propz and ammoprop_AltUse) = 0){check for dropping ammo from rope} then
           begin
           CurAmmoGear^.AmmoType:= CurAmmoType;
           Message:= Message or gmAttack;
           CurAmmoGear^.Message:= Message
           end else begin
           if not CurrentTeam^.ExtDriven and
             ((Ammoz[CurAmmoType].Ammo.Propz and ammoprop_Power) <> 0) then SendIPC('a');
           AfterAttack;
           end
        end else Message:= Message and not gmAttack;
     end
end;

procedure AfterAttack;
var s: shortstring;
    a: TAmmoType;
begin
with CurrentHedgehog^.Gear^,
        CurrentHedgehog^ do
    begin
    a:= CurAmmoType;
    State:= State and not gstAttacking;
    if (Ammoz[a].Ammo.Propz and ammoprop_Effect) = 0 then
        begin
        Inc(MultiShootAttacks);
        
        if (Ammoz[a].Ammo.NumPerTurn >= MultiShootAttacks) then
            begin
            s:= inttostr(Ammoz[a].Ammo.NumPerTurn - MultiShootAttacks + 1);
            AddCaption(format(trmsg[sidRemaining], s), cWhiteColor, capgrpAmmostate);
            end;
        
        if (Ammoz[a].Ammo.NumPerTurn >= MultiShootAttacks) or
            ((GameFlags and gfMultiWeapon) <> 0) then
            begin
            isInMultiShoot:= true
            end
        else
            begin
            OnUsedAmmo(CurrentHedgehog^);
            if ((Ammoz[a].Ammo.Propz and ammoprop_NoRoundEnd) = 0) and ((GameFlags and gfInfAttack) = 0) then
                TurnTimeLeft:= Ammoz[a].TimeAfterTurn;
            if ((Ammoz[a].Ammo.Propz and ammoprop_NoRoundEnd) = 0) then State:= State or gstAttacked;
            if (Ammoz[a].Ammo.Propz and ammoprop_NoRoundEnd) <> 0 then ApplyAmmoChanges(CurrentHedgehog^)
            end;
        end
    else
        begin
        OnUsedAmmo(CurrentHedgehog^);
        ApplyAmmoChanges(CurrentHedgehog^);
        end;
    AttackBar:= 0
    end
end;

////////////////////////////////////////////////////////////////////////////////
procedure doStepHedgehogDead(Gear: PGear);
const frametime = 200;
      timertime = frametime * 6;
begin
if Gear^.Hedgehog^.Unplaced then exit;
if Gear^.Timer > 1 then
    begin
    AllInactive:= false;
    dec(Gear^.Timer);
    if (Gear^.Timer mod frametime) = 0 then inc(Gear^.Pos)
    end else
if Gear^.Timer = 1 then
    begin
    Gear^.State:= Gear^.State or gstNoDamage;
    doMakeExplosion(hwRound(Gear^.X), hwRound(Gear^.Y), 30, EXPLAutoSound);
    AddGear(hwRound(Gear^.X), hwRound(Gear^.Y), gtGrave, 0, _0, _0, 0)^.Hedgehog:= Gear^.Hedgehog;
    DeleteGear(Gear);
    SetAllToActive
    end else // Gear^.Timer = 0
    begin
    AllInactive:= false;
    Gear^.Z:= cCurrHHZ;
    RemoveGearFromList(Gear);
    InsertGearToList(Gear);
    PlaySound(sndByeBye, Gear^.Hedgehog^.Team^.voicepack);
    Gear^.Pos:= 0;
    Gear^.Timer:= timertime
    end
end;

////////////////////////////////////////////////////////////////////////////////
procedure doStepHedgehogGone(Gear: PGear);
const frametime = 65;
      timertime = frametime * 11;
begin
if Gear^.Hedgehog^.Unplaced then exit;
if Gear^.Timer > 1 then
    begin
    AllInactive:= false;
    dec(Gear^.Timer);
    if (Gear^.Timer mod frametime) = 0 then inc(Gear^.Pos)
    end else
if Gear^.Timer = 1 then
    begin
    DeleteGear(Gear);
    SetAllToActive
    end else // Gear^.Timer = 0
    begin
    AllInactive:= false;
    Gear^.Z:= cCurrHHZ;
    RemoveGearFromList(Gear);
    InsertGearToList(Gear);
    PlaySound(sndByeBye, Gear^.Hedgehog^.Team^.voicepack);
    PlaySound(sndWarp);
    Gear^.Pos:= 0;
    Gear^.Timer:= timertime
    end
end;

////////////////////////////////////////////////////////////////////////////////
procedure PickUp(HH, Gear: PGear);
var s: shortstring;
    a: TAmmoType;
    i: LongInt;
    vga: PVisualGear;
begin
Gear^.Message:= gmDestroy;
PlaySound(sndShotgunReload);
case Gear^.Pos of
       posCaseUtility,
       posCaseAmmo: begin
                    a:= Gear^.AmmoType;
                    AddAmmo(HH^.Hedgehog^, a);
// Possibly needs to check shared clan ammo game flag once added.
// On the other hand, no obvious reason that clan members shouldn't know what ammo another clan member picked up
                    if (not (HH^.Hedgehog^.Team^.ExtDriven 
                      or (HH^.Hedgehog^.BotLevel > 0)))
                      or (HH^.Hedgehog^.Team^.Clan^.ClanIndex = LocalClan)
                      or (GameType = gmtDemo)  then
                        begin
                        s:= trammo[Ammoz[a].NameId] + ' (+' + IntToStr(Ammoz[a].NumberInCase) + ')';
                        AddCaption(s, HH^.Hedgehog^.Team^.Clan^.Color, capgrpAmmoinfo);

                        // show ammo icon
                        vga:= AddVisualGear(hwRound(Gear^.X), hwRound(Gear^.Y), vgtAmmo);
                        if vga <> nil then
                            vga^.Frame:= Longword(a);
                        end;

                    end;
     posCaseHealth: begin
                    inc(HH^.Health, Gear^.Health);
                    HH^.Hedgehog^.Effects[hePoisoned] := false;
                    str(Gear^.Health, s);
                    s:= '+' + s;
                    AddCaption(s, HH^.Hedgehog^.Team^.Clan^.Color, capgrpAmmoinfo);
                    RenderHealth(HH^.Hedgehog^);
                    RecountTeamHealth(HH^.Hedgehog^.Team);

                    i:= 0;
                    while i < Gear^.Health do
                        begin
                        AddVisualGear(hwRound(HH^.X), hwRound(HH^.Y), vgtHealth);
                        inc(i, 5);
                        end;
                    end;
     end
end;

const StepTicks: LongWord = 0;

procedure HedgehogStep(Gear: PGear);
var PrevdX: LongInt;
    CurWeapon: PAmmo;
begin
CurWeapon:= GetAmmoEntry(Gear^.Hedgehog^);
if ((Gear^.State and (gstAttacking or gstMoving)) = 0) then
   begin
   if isCursorVisible then
      with Gear^.Hedgehog^ do
        with CurWeapon^ do
          begin
          if (Gear^.Message and gmLeft  ) <> 0 then
             Pos:= (Pos - 1 + Ammoz[AmmoType].PosCount) mod Ammoz[AmmoType].PosCount
          else
          if (Gear^.Message and gmRight ) <> 0 then
             Pos:= (Pos + 1) mod Ammoz[AmmoType].PosCount
          else exit;
          StepTicks:= 200;
          exit
          end;

    if ((Gear^.Message and gmAnimate) <> 0) then
        begin
        Gear^.Message:= 0;
        Gear^.State:= Gear^.State or gstAnimation;
        Gear^.Tag:= Gear^.MsgParam;
        Gear^.Timer:= 0;
        Gear^.Pos:= 0
        end;

   if ((Gear^.Message and gmLJump ) <> 0) then
      begin
      Gear^.Message:= Gear^.Message and not gmLJump;
      DeleteCI(Gear);
      if not TestCollisionYwithGear(Gear, -1) then
         if not TestCollisionXwithXYShift(Gear, _0, -2, hwSign(Gear^.dX)) then Gear^.Y:= Gear^.Y - _2 else
         if not TestCollisionXwithXYShift(Gear, _0, -1, hwSign(Gear^.dX)) then Gear^.Y:= Gear^.Y - _1;
      if not (TestCollisionXwithGear(Gear, hwSign(Gear^.dX))
         or   TestCollisionYwithGear(Gear, -1)) then
         begin
         Gear^.dY:= -_0_15;
         if not cArtillery then Gear^.dX:= SignAs(_0_15, Gear^.dX);
         Gear^.State:= Gear^.State or gstMoving or gstHHJumping;
         PlaySound(sndJump1, Gear^.Hedgehog^.Team^.voicepack);
         exit
         end;
      end;

   if ((Gear^.Message and gmHJump ) <> 0) then
      begin
      DeleteCI(Gear);
      Gear^.Message:= Gear^.Message and not gmHJump;

      Gear^.dY:= -_0_2;
      SetLittle(Gear^.dX);
      Gear^.State:= Gear^.State or gstMoving or gstHHJumping;
      PlaySound(sndJump3, Gear^.Hedgehog^.Team^.voicepack);
      exit
      end;

   PrevdX:= hwSign(Gear^.dX);
   if (Gear^.Message and gmLeft  )<>0 then Gear^.dX:= -cLittle else
   if (Gear^.Message and gmRight )<>0 then Gear^.dX:=  cLittle else exit;

   if (Gear^.Message and (gmLeft or gmRight)) <> 0 then
      begin
      StepSoundTimer:= cHHStepTicks;
      end;
   
   StepTicks:= cHHStepTicks;
   if PrevdX <> hwSign(Gear^.dX) then
      begin
      FollowGear:= Gear;
      exit
      end;
   DeleteCI(Gear); // must be after exit!! (see previous line)

   Gear^.Hedgehog^.visStepPos:= (Gear^.Hedgehog^.visStepPos + 1) and 7;
   if TestCollisionXwithGear(Gear, hwSign(Gear^.dX)) then
      begin
      if not (TestCollisionXwithXYShift(Gear, _0, -6, hwSign(Gear^.dX))
         or TestCollisionYwithGear(Gear, -1)) then Gear^.Y:= Gear^.Y - _1;
      if not (TestCollisionXwithXYShift(Gear, _0, -5, hwSign(Gear^.dX))
         or TestCollisionYwithGear(Gear, -1)) then Gear^.Y:= Gear^.Y - _1;
      if not (TestCollisionXwithXYShift(Gear, _0, -4, hwSign(Gear^.dX))
         or TestCollisionYwithGear(Gear, -1)) then Gear^.Y:= Gear^.Y - _1;
      if not (TestCollisionXwithXYShift(Gear, _0, -3, hwSign(Gear^.dX))
         or TestCollisionYwithGear(Gear, -1)) then Gear^.Y:= Gear^.Y - _1;
      if not (TestCollisionXwithXYShift(Gear, _0, -2, hwSign(Gear^.dX))
         or TestCollisionYwithGear(Gear, -1)) then Gear^.Y:= Gear^.Y - _1;
      if not (TestCollisionXwithXYShift(Gear, _0, -1, hwSign(Gear^.dX))
         or TestCollisionYwithGear(Gear, -1)) then Gear^.Y:= Gear^.Y - _1;
      end;

   if (not cArtillery) and ((Gear^.Message and gmPrecise) = 0) and (not TestCollisionXwithGear(Gear, hwSign(Gear^.dX))) then
      Gear^.X:= Gear^.X + SignAs(_1, Gear^.dX);

   SetAllHHToActive;

   if not TestCollisionYwithGear(Gear, 1) then
   begin
   Gear^.Y:= Gear^.Y + _1;
   if not TestCollisionYwithGear(Gear, 1) then
   begin
   Gear^.Y:= Gear^.Y + _1;
   if not TestCollisionYwithGear(Gear, 1) then
   begin
   Gear^.Y:= Gear^.Y + _1;
   if not TestCollisionYwithGear(Gear, 1) then
   begin
   Gear^.Y:= Gear^.Y + _1;
   if not TestCollisionYwithGear(Gear, 1) then
   begin
   Gear^.Y:= Gear^.Y + _1;
   if not TestCollisionYwithGear(Gear, 1) then
   begin
   Gear^.Y:= Gear^.Y + _1;
   if not TestCollisionYwithGear(Gear, 1) then
      begin
      Gear^.Y:= Gear^.Y - _6;
      Gear^.dY:= _0;
      Gear^.State:= Gear^.State or gstMoving;
      exit
      end;
   end
   end
   end
   end
   end
   end;
   AddGearCI(Gear)
   end
end;

procedure HedgehogChAngle(Gear: PGear);
var da: LongWord;
begin
with Gear^.Hedgehog^ do
    if (CurAmmoType = amRope)
    and ((Gear^.State and (gstMoving or gstHHJumping)) = gstMoving) then da:= 2 else da:= 1;

if (((Gear^.Message and gmPrecise) = 0) or ((GameTicks mod 5) = 1)) then
    if ((Gear^.Message and gmUp) <> 0) and (Gear^.Angle >= CurMinAngle + da) then dec(Gear^.Angle, da)
    else
    if ((Gear^.Message and gmDown) <> 0) and (Gear^.Angle + da <= CurMaxAngle) then inc(Gear^.Angle, da)
end;

procedure doStepHedgehog(Gear: PGear); forward;
////////////////////////////////////////////////////////////////////////////////
procedure doStepHedgehogMoving(Gear: PGear);
var isFalling, isUnderwater: boolean;
begin
isUnderwater:= cWaterLine < hwRound(Gear^.Y) + Gear^.Radius;
if Gear^.dX.QWordValue > 8160437862 then Gear^.dX.QWordValue:= 8160437862;
if Gear^.dY.QWordValue > 8160437862 then Gear^.dY.QWordValue:= 8160437862;

if Gear^.Hedgehog^.Unplaced then
   begin
   Gear^.dY:= _0;
   Gear^.dX:= _0;
   Gear^.State:= Gear^.State and not gstMoving;
   exit
   end;
isFalling:= (Gear^.dY.isNegative) or not TestCollisionYKick(Gear, 1);
if isFalling then
   begin
   if (Gear^.dY.isNegative) and TestCollisionYKick(Gear, -1) then Gear^.dY:= _0;
   Gear^.State:= Gear^.State or gstMoving;
   if isUnderwater then Gear^.dY:= Gear^.dY + cGravity / _2
   else
       begin
       Gear^.dY:= Gear^.dY + cGravity;
// this set of circumstances could be less complex if jumping was more clearly identified
       if ((GameFlags and gfMoreWind) <> 0) and 
          (((Gear^.Damage <> 0) or
          ((CurAmmoGear <> nil) and
            ((CurAmmoGear^.AmmoType = amJetpack) or
            (CurAmmoGear^.AmmoType = amBirdy))) or
          ((Gear^.dY.QWordValue + Gear^.dX.QWordValue) > _0_55.QWordValue)))
          then Gear^.dX := Gear^.dX + cWindSpeed / Gear^.Density
       end
   end 
else
   begin
   if ((Gear^.dX.QWordValue + Gear^.dY.QWordValue) < _0_55.QWordValue)
      and ((Gear^.State and gstHHJumping) <> 0) then SetLittle(Gear^.dX);

   if not Gear^.dY.isNegative then
      begin
      CheckHHDamage(Gear);

      if ((Gear^.State and gstHHHJump) <> 0) and (not cArtillery) and
         (Gear^.dX.QWordValue < _0_02.QWordValue) then Gear^.dX.isNegative:= not Gear^.dX.isNegative; // landing after high jump

      Gear^.State:= Gear^.State and not (gstHHJumping or gstHHHJump);
      Gear^.dY:= _0;
      end else Gear^.dY:= Gear^.dY + cGravity;

   if ((Gear^.State and gstMoving) <> 0) then Gear^.dX:= Gear^.dX * Gear^.Friction
   end;

if (Gear^.State <> 0) then DeleteCI(Gear);

if isUnderwater then
   begin
   Gear^.dY:= Gear^.dY * _0_999;
   Gear^.dX:= Gear^.dX * _0_999;
   end;

if (Gear^.State and gstMoving) <> 0 then
   if TestCollisionXKick(Gear, hwSign(Gear^.dX)) then
      if not isFalling then
         if hwAbs(Gear^.dX) > _0_01 then
            if not TestCollisionXwithXYShift(Gear, int2hwFloat(hwSign(Gear^.dX)) - Gear^.dX, -1, hwSign(Gear^.dX)) then begin Gear^.X:= Gear^.X + Gear^.dX; Gear^.dX:= Gear^.dX * _0_96; Gear^.Y:= Gear^.Y - _1 end else
            if not TestCollisionXwithXYShift(Gear, int2hwFloat(hwSign(Gear^.dX)) - Gear^.dX, -2, hwSign(Gear^.dX)) then begin Gear^.X:= Gear^.X + Gear^.dX; Gear^.dX:= Gear^.dX * _0_93; Gear^.Y:= Gear^.Y - _2 end else
            if not TestCollisionXwithXYShift(Gear, int2hwFloat(hwSign(Gear^.dX)) - Gear^.dX, -3, hwSign(Gear^.dX)) then begin Gear^.X:= Gear^.X + Gear^.dX; Gear^.dX:= Gear^.dX * _0_9 ; Gear^.Y:= Gear^.Y - _3 end else
            if not TestCollisionXwithXYShift(Gear, int2hwFloat(hwSign(Gear^.dX)) - Gear^.dX, -4, hwSign(Gear^.dX)) then begin Gear^.X:= Gear^.X + Gear^.dX; Gear^.dX:= Gear^.dX * _0_87; Gear^.Y:= Gear^.Y - _4 end else
            if not TestCollisionXwithXYShift(Gear, int2hwFloat(hwSign(Gear^.dX)) - Gear^.dX, -5, hwSign(Gear^.dX)) then begin Gear^.X:= Gear^.X + Gear^.dX; Gear^.dX:= Gear^.dX * _0_84; Gear^.Y:= Gear^.Y - _5 end else
            if hwAbs(Gear^.dX) > _0_02 then Gear^.dX:= -Gear^.Elasticity * Gear^.dX
                                   else begin
                                        Gear^.State:= Gear^.State and not gstMoving;
                                        SetLittle(Gear^.dX)
                                        end
            else begin
                 Gear^.State:= Gear^.State and not gstMoving;
                 SetLittle(Gear^.dX)
                 end
         else if (hwAbs(Gear^.dX) > cLittle)
                and ((Gear^.State and gstHHJumping) = 0)
                then Gear^.dX:= -Gear^.Elasticity * Gear^.dX
                else SetLittle(Gear^.dX);

if (not isFalling) and
   (hwAbs(Gear^.dX) + hwAbs(Gear^.dY) < _0_03) then
   begin
   Gear^.State:= Gear^.State and not gstWinner;
   Gear^.State:= Gear^.State and not gstMoving;
   SetLittle(Gear^.dX);
   Gear^.dY:= _0
   end else Gear^.State:= Gear^.State or gstMoving;

if (Gear^.State and gstMoving) <> 0 then
   begin
   Gear^.State:= Gear^.State and not gstAnimation;
// ARTILLERY but not being moved by explosions
   Gear^.X:= Gear^.X + Gear^.dX;
   Gear^.Y:= Gear^.Y + Gear^.dY;
   if (not Gear^.dY.isNegative) and
      (not TestCollisionYKick(Gear, 1)) and
       TestCollisionYwithXYShift(Gear, 0, 1, 1) then
      begin
      CheckHHDamage(Gear);
      Gear^.dY:= _0;
      Gear^.Y:= Gear^.Y + _1
      end;
   CheckGearDrowning(Gear);
   if (Gear^.State and gstDrowning) <> 0 then isCursorVisible:= false
   end;

if (hwAbs(Gear^.dY) > _0) and (Gear^.FlightTime > 0) and ((GameFlags and gfLowGravity) = 0) then
    begin
    inc(Gear^.FlightTime, 1);
    if Gear^.FlightTime = 3000 then
        begin
        AddCaption(GetEventString(eidHomerun), cWhiteColor, capgrpMessage);
        PlaySound(sndHomerun)
        end;
    end
else
    begin
    Gear^.FlightTime:= 0;
    end;

end;

procedure doStepHedgehogDriven(Gear: PGear);
var t: PGear;
    wasJumping: boolean;
    Hedgehog: PHedgehog;
begin
Hedgehog:= Gear^.Hedgehog;
if not isInMultiShoot then
   AllInactive:= false
else
   Gear^.Message:= 0;

if (TurnTimeLeft = 0) or (Gear^.Damage > 0) then
    begin
    TurnTimeLeft:= 0;
    isCursorVisible:= false;
    Gear^.State:= Gear^.State and not (gstHHDriven or gstAnimation or gstAttacking);
    AttackBar:= 0;
    if Gear^.Damage > 0 then
        Gear^.State:= Gear^.State and not (gstHHJumping or gstHHHJump);
    exit
    end;

if (Gear^.State and gstAnimation) <> 0 then
    begin
    Gear^.Message:= 0;
    if (Gear^.Pos = Wavez[TWave(Gear^.Tag)].VoiceDelay) and (Gear^.Timer = 0) then PlaySound(Wavez[TWave(Gear^.Tag)].Voice, Hedgehog^.Team^.voicepack);
    inc(Gear^.Timer);
    if Gear^.Timer = Wavez[TWave(Gear^.Tag)].Interval then
        begin
        Gear^.Timer:= 0;
        inc(Gear^.Pos);
        if Gear^.Pos = Wavez[TWave(Gear^.Tag)].FramesCount then
            Gear^.State:= Gear^.State and not gstAnimation
        end;
    exit
    end;

if ((Gear^.State and gstMoving) <> 0)
    or (StepTicks = cHHStepTicks)
    or (CurAmmoGear <> nil) then // we are moving
    begin
    with Hedgehog^ do
        if (CurAmmoGear = nil)
        and (Gear^.dY > _0_39)
        and (CurAmmoType = amParachute) then Gear^.Message:= Gear^.Message or gmAttack;
    // check for case with ammo
    t:= CheckGearNear(Gear, gtCase, 36, 36);
    if t <> nil then
        PickUp(Gear, t)
    end;

if (CurAmmoGear = nil) then
    if (((Gear^.Message and gmAttack) <> 0)
        or ((Gear^.State and gstAttacking) <> 0)) then
        Attack(Gear) // should be before others to avoid desync with '/put' msg and changing weapon msgs
    else
else 
    with Hedgehog^ do
        if ((Ammoz[CurAmmoGear^.AmmoType].Ammo.Propz and ammoprop_AltAttack) <> 0)
            and ((Gear^.Message and gmLJump) <> 0)
            and ((Ammoz[CurAmmoType].Ammo.Propz and ammoprop_AltUse) <> 0) then
            begin
            Attack(Gear);
            Gear^.Message:= Gear^.Message and not gmLJump
            end;

if (CurAmmoGear = nil)
    or ((Ammoz[CurAmmoGear^.AmmoType].Ammo.Propz and ammoprop_AltAttack) <> 0) 
    or ((Ammoz[CurAmmoGear^.AmmoType].Ammo.Propz and ammoprop_NoRoundEnd) <> 0) then
    begin
    if ((Gear^.Message and gmSlot) <> 0) then
        if ChangeAmmo(Gear) then ApplyAmmoChanges(Hedgehog^);

    if ((Gear^.Message and gmWeapon) <> 0) then HHSetWeapon(Gear);

    if ((Gear^.Message and gmTimer) <> 0) then HHSetTimer(Gear);
    end;

if CurAmmoGear <> nil then
   begin
   CurAmmoGear^.Message:= Gear^.Message;
   exit
   end;

if not isInMultiShoot then
   HedgehogChAngle(Gear);

if (Gear^.State and gstMoving) <> 0 then
    begin
    wasJumping:= ((Gear^.State and gstHHJumping) <> 0);

    if ((Gear^.Message and gmHJump) <> 0) and
        wasJumping and
        ((Gear^.State and gstHHHJump) = 0) then
        if (not (hwAbs(Gear^.dX) > cLittle)) and (Gear^.dY < -_0_02) then
            begin
            Gear^.State:= Gear^.State or gstHHHJump;
            Gear^.dY:= -_0_25;
            if not cArtillery then Gear^.dX:= -SignAs(_0_02, Gear^.dX);
            PlaySound(sndJump2, Hedgehog^.Team^.voicepack)
            end;

    Gear^.Message:= Gear^.Message and not (gmLJump or gmHJump);

    if (not cArtillery) and wasJumping and
        TestCollisionXwithGear(Gear, hwSign(Gear^.dX)) then SetLittle(Gear^.dX);

    doStepHedgehogMoving(Gear);

    if ((Gear^.State and (gstMoving or gstDrowning)) = 0) then
        begin
        AddGearCI(Gear);
        if wasJumping then
            StepTicks:= 410
        else
            StepTicks:= 95
        end;
    exit
    end;

    if not isInMultiShoot then
        begin
        if StepTicks > 0 then dec(StepTicks);
        if (StepTicks = 0) then HedgehogStep(Gear)
        end
end;

////////////////////////////////////////////////////////////////////////////////
procedure doStepHedgehogFree(Gear: PGear);
var prevState: Longword;
begin
prevState:= Gear^.State;

doStepHedgehogMoving(Gear);

if (Gear^.State and (gstMoving or gstDrowning)) <> 0 then
    begin
    if Gear^.Damage > 0 then CalcRotationDirAngle(Gear);
    AllInactive:= false;
    exit
    end;

if (Gear^.Health = 0) then
    begin
    if PrvInactive or ((GameFlags and gfInfAttack) <> 0) then
        begin
        Gear^.Timer:= 0;
        FollowGear:= Gear;
        PrvInactive:= false;
        AllInactive:= false;

        if not Gear^.Hedgehog^.Team^.hasGone then
            begin
            Gear^.Hedgehog^.Effects[hePoisoned] := false;
            if Gear^.Hedgehog^.Effects[heResurrectable] then begin
                ResurrectHedgehog(Gear);
            end else begin
                Gear^.State:= Gear^.State or gstHHDeath;
                Gear^.doStep:= @doStepHedgehogDead;
                // Death message
                AddCaption(Format(GetEventString(eidDied), Gear^.Hedgehog^.Name), cWhiteColor, capgrpMessage);
            end;
            end
        else
            begin
            Gear^.State:= Gear^.State or gstHHGone;
            Gear^.doStep:= @doStepHedgehogGone;
            // Gone message
            AddCaption(Format(GetEventString(eidGone), Gear^.Hedgehog^.Name), cWhiteColor, capgrpMessage);
            end
        end;
    exit
    end;

if ((Gear^.State and gstWait) = 0) and
    (prevState <> Gear^.State) then
    begin
    Gear^.State:= gstWait;
    Gear^.Timer:= 150
    end else
    begin
    if Gear^.Timer = 0 then
        begin
        Gear^.State:= 0;
        Gear^.Active:= false;
        AddGearCI(Gear);
        exit
        end else dec(Gear^.Timer)
    end;

AllInactive:= false
end;

////////////////////////////////////////////////////////////////////////////////
procedure doStepHedgehog(Gear: PGear);
begin
if (Gear^.Message and gmDestroy) <> 0 then
    begin
    DeleteGear(Gear);
    exit
    end;

if (Gear^.State and gstHHDriven) = 0 then
    doStepHedgehogFree(Gear)
else
    begin
    with Gear^.Hedgehog^ do
        if Team^.hasGone then TeamGoneEffect(Team^);
    doStepHedgehogDriven(Gear)
    end;
end;