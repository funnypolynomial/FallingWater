#!/usr/bin/env python
'''
Fallingwater enclosure
TroGlass Sky Blue 3mm

Note that I'm going to some trouble to render the design realistically, with plates drawn in blue, and with transparent voids (in DEVEL)
Holes joining plates from the front are M2.5
Holes attaching LCD are (a little bigger than) M2
Holes joining the two back plates via standoffs are M3
Holes joining PCB to back plate are M3
'''

import inkex
from inksnek import *
import sys
import sys
sys.path.append('C:/inksnek/extras/')
from read_gerber_holes import *

class MyDesign(inkex.Effect):
  def __init__(self):
    inkex.Effect.__init__(self)

  def pathHole(self, x, y, r, clockwise, group):
    # add as a *path* (anti-)clock, 4 arcs (if rendering)
    if inksnek.mode == inksnek.PRINT: # mark drill point
        inksnek.add_X_marker(group, x, y)
    if self.render:
      path = inksnek.path_move_to(x, y + r)
      if clockwise:
        path += inksnek.path_round_by(+r, -r, r)
        path += inksnek.path_round_by(-r, -r, r)
        path += inksnek.path_round_by(-r, +r, r)
        path += inksnek.path_round_by(+r, +r, r)
      else:
        path += inksnek.path_round_by(-r, -r, -r)
        path += inksnek.path_round_by(+r, -r, -r)
        path += inksnek.path_round_by(+r, +r, -r)
        path += inksnek.path_round_by(-r, +r, -r)
      return path
    else:
      inksnek.add_hole(group, x, y, r)
      if inksnek.mode == inksnek.DEVEL: # show the extent of the head
        headR = self.plateScrewHeadDiameter/2.0 if r == self.plateHoleDiameter/2.0 else self.m3ScrewHeadDiameter/2.0
        inksnek.add_circle(group, x, y, headR, inksnek.ignore_style)
      return ""
    
  def plate1Holes(self, group, diameter, dX = 0.0, dY = 0.0):
    # return just the path with the holes, clockwise
    x,y = self.holePositions[0]
    path = self.pathHole(x - dX, y + dY, diameter/2.0, True, group)
    path += self.pathHole(-x + dX, y + dY, diameter/2.0, True, group)
    x,y = self.holePositions[1]
    path += self.pathHole(x - dX, y - dY, diameter/2.0, True, group)
    path += self.pathHole(-x + dX, y - dY, diameter/2.0, True, group)
    return path

  def plate2Holes(self, dX, group):
    # return just the path with the holes, clockwise
    x,y = self.holePositions[0]
    path = self.pathHole(x - dX, y, self.plateHoleDiameter/2.0, True, group)
    x,y = self.holePositions[1]
    path += self.pathHole(x - dX, y, self.plateHoleDiameter/2.0, True, group)
    return path
    
  def plate3Holes(self, dX, group):
    return self.plate2Holes(dX, group)    
  
  def addLCDBoard(self, group):
    # outline of the LCD board and mount through-holes
    board = inksnek.add_group(group, inksnek.translate_group(-self.lcdBoardWidth/2.0, (self.plate1Height-self.lcdVoidHeight)/2.0 + self.lcdVoidMargin - self.lcdDisplayBottomOffset))
    # board outline
    inksnek.add_rect(board, 0.0, 0.0, self.lcdBoardWidth, self.lcdBoardHeight, inksnek.ignore_style)
    # backlight protrusion
    inksnek.add_rect(group, -self.lcdBacklightWidth/2.0, (self.plate1Height + self.lcdVoidHeight)/2.0, self.lcdBacklightWidth, self.lcdBacklightHeight, inksnek.ignore_style, "TLR")
    # holes
    if self.lcdBoardHoles:
        holeStyle = self.holeStyle
        inksnek.add_circle(board, self.lcdBoardHoleOffsetX, self.lcdBoardHoleOffsetY, self.lcdBoardHoleD/2.0, holeStyle)
        inksnek.add_circle(board, self.lcdBoardWidth - self.lcdBoardHoleOffsetX, self.lcdBoardHoleOffsetY, self.lcdBoardHoleD/2.0, holeStyle)
        inksnek.add_circle(board, self.lcdBoardHoleOffsetX, self.lcdBoardHeight - self.lcdBoardHoleOffsetY, self.lcdBoardHoleD/2.0, holeStyle)
        inksnek.add_circle(board, self.lcdBoardWidth - self.lcdBoardHoleOffsetX, self.lcdBoardHeight - self.lcdBoardHoleOffsetY, self.lcdBoardHoleD/2.0, holeStyle)
      
  def addPlate1(self, group):
    # main/front central plate
    # origin of plate is middle of bottom edge
    void = [ # based in OpenSCAD version
      [-self.lcdVoidWidth/2.0, 0.0],
      [+self.lcdVoidWidth/2.0, 0.0], 
      [+self.lcdVoidWidth/2.0, self.lcdVoidHeight],
      [-self.lcdVoidWidth/2.0, self.lcdVoidHeight],
    ]
    plate = [
      [[-self.plate1WidthBase/2.0, 0.0]],
      [+self.plate1WidthBase/2.0, 0.0], 
      [+self.plate1WidthTop/2.0, self.plate1Height],
        [0.0, self.plate1Height + self.plate1Peak],  # peak/valley todo, half the plate2 slope?
      [-self.plate1WidthTop/2.0, self.plate1Height],
      []
    ]
    void.reverse()     # so it's a hole in the fill
    void[0] =[void[0]] # start with a move
    void.append([])    # close
    path = inksnek.shape_to_path(0.0, 0.0, 1.0, 1.0, plate) + inksnek.shape_to_path(0.0, (self.plate1Height - self.lcdVoidHeight)/2.0, 1.0, 1.0, void)
    path += self.plate1Holes(group, self.plateHoleDiameter)
    inksnek.add_path(group, path, self.plateStyle)
    self.addLCDBoard(group)
    self.addLCDSpacer(group, 1.0 - self.lcdVoidWidth/2.0, 1.0 + (self.plate1Height - self.lcdVoidHeight)/2.0)

  def addPlate2(self, group, dX):
    # middle plate
    # right hand. origin is bottom corner (excluding overlap), anti-clockwise
    plate = [
      [[-self.plateOverlap, 0]],
      [0, 0],
      [self.plate2Radius*cos(self.plate2Angle), self.plate2Radius*sin(self.plate2Angle)],
      [self.plate2Radius*cos(self.plate1Angle), self.plate2Radius*sin(self.plate1Angle)],   
      [self.plate2Radius*cos(self.plate1Angle) - self.plateOverlap, self.plate2Radius*sin(self.plate1Angle)],
      []
    ]
    path = inksnek.shape_to_path(0.0, 0.0, 1.0, 1.0, plate)
    path += self.plate2Holes(dX, group)
    inksnek.add_path(group, path, self.plateStyle)
   
  def addPlate3(self, group, dX, RHS):
    # back plate
    # right hand. origin is bottom corner (excluding overlap)
    if True: 
      # extend to top corner of plate2? common hole
      plate = [
        [[-self.plateOverlap, 0]],
        [self.plate3Radius*cos(self.plate3Angle), self.plate3Radius*sin(self.plate3Angle)],
          [self.plate2Radius*cos(self.plate1Angle), self.plate2Radius*sin(self.plate1Angle)],   
          [self.plate2Radius*cos(self.plate1Angle) - self.plateOverlap, self.plate2Radius*sin(self.plate1Angle)],
        []
      ]
    else:
      plate = [
        [[-self.plateOverlap, 0]],
        [self.plate3Radius*cos(self.plate3Angle), self.plate3Radius*sin(self.plate3Angle)],
        [self.plate3Radius*cos(self.plate2Angle), self.plate3Radius*sin(self.plate2Angle)],   
        [self.plate3Radius*cos(self.plate2Angle) - self.plateOverlap, self.plate3Radius*sin(self.plate2Angle)],
        [-self.plateOverlap, self.plateOverlap],
        []
      ]
    path = inksnek.shape_to_path(0.0, 0.0, 1.0, 1.0, plate)
    path += self.plate3Holes(dX, group)
    inksnek.add_path(group, path, self.plateStyle)
    #if RHS:  it's on the PCB
    #  inksnek.add_annotation(group, self.plate3Radius/2.0, self.plateOverlap/2.0, 
    #    "MEW MMXXIV", 2.0, self.lineStyle, inksnek.CENTRE_ALIGN)
        
  def addPlateSpacer(self, group, dX):
    # Extra space between plate2 and back plate to make room for LCD
    plate = [
      [[-self.plateOverlap, 0]],
      [0, 0],
      [self.plate2Radius*cos(self.plate1Angle), self.plate2Radius*sin(self.plate1Angle)],   
      [self.plate2Radius*cos(self.plate1Angle) - self.plateOverlap, self.plate2Radius*sin(self.plate1Angle)],
      []
    ]
    path = inksnek.shape_to_path(0.0, 0.0, 1.0, 1.0, plate)
    path += self.plate2Holes(dX, group)
    inksnek.add_path(group, path, self.plateStyle)
   
  
  def addLCDSpacer(self, group, x, y):
    # for the LCD to rest on
    if not self.render:
        spacer = inksnek.add_group(group, inksnek.translate_group(x, y))
        inksnek.add_rect(spacer, 0, 0, 2.0*self.spacerWidth, self.spacerHeight, inksnek.cut_style)
        inksnek.add_rect(spacer, self.spacerWidth - self.spacerCutoutWidth, (self.spacerHeight-self.spacerCutoutHeight)/2.0, 2.0*self.spacerCutoutWidth, self.spacerCutoutHeight, inksnek.cut_style)
        inksnek.add_line_by(spacer, self.spacerWidth, 0, 0, self.spacerHeight, inksnek.cut_style)

  def addBoardPlate(self, group, outer):
    # the plates to hold the board
    ht = self.plate2Radius*sin(self.plate1Angle)
    peak = self.plate1Peak/2 if outer else self.plate1Peak
    plate = [ # clockwise. see addPlate1
      [[-self.plate1WidthBase/2.0, 0.0]],
      [+self.plate1WidthBase/2.0, 0.0], 
      [+(self.plate2Radius*cos(self.plate1Angle) + self.plate1WidthBase/2.0), ht],
        [0.0, ht + peak], 
      [-(self.plate2Radius*cos(self.plate1Angle) + self.plate1WidthBase/2.0), ht],
      []
    ]
    path = inksnek.shape_to_path(0.0, 0.0, 1.0, 1.0, plate)
    if not outer:
        # inner plate
        path += self.plate1Holes(group, self.plateHoleDiameter)
        # window for LCD leads
        x = -self.lcdBoardWidth/2.0 # see addLCDBoard
        y = (self.plate1Height-self.lcdVoidHeight)/2.0 + self.lcdVoidMargin - self.lcdDisplayBottomOffset
        ht = 7.0
        path += inksnek.path_move_to(x, y + ht) # anti-clock
        path += inksnek.path_horz_by(self.lcdBoardWidth)
        path += inksnek.path_vert_by(-ht)
        path += inksnek.path_horz_by(-self.lcdBoardWidth)
        path += inksnek.path_close()
    # these holes join the two boards
    path += self.plate1Holes(group, 3.0, self.plateOverlap + 1.0, 1.0)
    inksnek.add_path(group, path, self.plateStyle)
    if outer:
        perfWidth = 28
        perfHeight = 16
        perfOffsetY = 3
        perfHoleR = 3.0/2.0
        inksnek.add_hole(group, -inksnek.on_perf_board(7), inksnek.on_perf_board(perfOffsetY + 2), perfHoleR)
        inksnek.add_hole(group, +inksnek.on_perf_board(7), inksnek.on_perf_board(perfOffsetY + 2), perfHoleR)
        inksnek.add_hole(group, -inksnek.on_perf_board(2), inksnek.on_perf_board(perfOffsetY + 14), perfHoleR)
        if inksnek.mode != inksnek.FINAL:
            inksnek.add_rect(group, -inksnek.on_perf_board(perfWidth/2), inksnek.on_perf_board(perfOffsetY), inksnek.on_perf_board(perfWidth), inksnek.on_perf_board(perfHeight), inksnek.ignore_style)
            inksnek.add_perf_board(group, -inksnek.on_perf_board(perfWidth/2), inksnek.on_perf_board(perfOffsetY), perfWidth + 1, perfHeight + 1)
            hole_style = inksnek.create_fill_style("#AAAAAA", 0.75)
            gerber = read_gerber_holes()
            holes = gerber.read_zip_file("Gerber_FallingWater.zip")
            for hole in holes:
                inksnek.add_circle(group, hole[0] - inksnek.on_perf_board(perfWidth/2), hole[1] + inksnek.on_perf_board(perfOffsetY), hole[2]/2, hole_style)
    
  def effect(self):  # the main entry point for the design
    # initialise Inksnek
    self.materialThickness = 3.0
    inksnek.setup(self, inksnek.A4, inksnek.ACRYLIC, self.materialThickness, 'mm', inksnek.DEVEL)
    self.render = False # overlap and colour the plates etc to look assembled
    self.plateStyle = inksnek.cut_style
    self.holeStyle = inksnek.cut_style
    self.lineStyle = inksnek.etch_style
    if inksnek.mode == inksnek.DEVEL and self.render:
        self.plateStyle = inksnek.create_style("#000000", inksnek.line_width, "#0000FF", 0.75)
        self.holeStyle = inksnek.create_style("#000000", inksnek.line_width, "#FFFFFF", 1.0)
        self.lineStyle = inksnek.create_stroke_style("#8080FF", inksnek.line_width, 1.0)

    # design parameters (see https://www.aliexpress.com/item/1005005761527065.html)
    self.lcdBoardHoles = True
    self.lcdBoardWidth = 24
    self.lcdBoardHeight = 92
    self.lcdDisplayWidth = 19.0
    self.lcdDisplayHeight = 78.0
    self.lcdDisplayBottomOffset = 5.0 # bottom of board to bottom of LCD glass layer
    # the part of the backlight which protrudes
    self.lcdBacklightWidth = 15.0
    self.lcdBacklightHeight = 6.0
    self.lcdBoardHoleOffsetX = self.lcdBoardHoleOffsetY = 2.5
    self.lcdBoardHoleD = 2.25 # using m2  but leave some play
    

    # the gap the LCD sticks throught
    self.lcdVoidMargin = 0.25
    self.lcdVoidWidth = self.lcdDisplayWidth + 2.0*self.lcdVoidMargin
    self.lcdVoidHeight = self.lcdDisplayHeight + 2.0*self.lcdVoidMargin


    self.plateOverlap = 6
    self.golden = 1.61803399 # golden ratio
    self.xplode = 0 if self.render else 8
    self.epsilon = 0.00 # merge plates? (N/A outside OpenSCAD)

    self.plate1Height = 110.0
    self.plate1WidthBase = 40.0
    self.plate1WidthTop = self.golden*self.plate1WidthBase # 80.0
    self.plate1Peak = 5.0
    self.plate1Angle = atan2(self.plate1Height, (self.plate1WidthTop - self.plate1WidthBase)/2.0)
    self.plate1Radius = sqrt(self.plate1Height**2 + (self.plate1WidthTop-self.plate1WidthBase)**2)

    self.plate2Angle = 2*self.plate1Angle/3.0
    self.plate2Radius = self.plate1Radius/self.golden  # 3*self.plate1Radius/4

    self.plate3Angle = 0.0
    self.plate3Radius = self.plate2Radius/self.golden  # 3*self.plate2Radius/4

    self.plateHoleDiameter = 2.5 # m2.5
    self.plateScrewHeadDiameter = 4 # m2.5
    # just the two right-most holes, on plate1
    self.holePositions = [[self.plate1WidthBase/2.0 - self.plateOverlap/2.0, 
                           self.plateOverlap/2.0],
                          [self.plate1WidthBase/2.0 + self.plateOverlap/2.0 + self.plate2Radius*cos(self.plate1Angle) - self.plateOverlap, 
                           self.plate2Radius*sin(self.plate1Angle) - self.plateOverlap/2.0]]
 
    # two strips the LCD board rests on
    self.spacerHeight = 75
    self.spacerWidth = 5
    self.spacerCutoutHeight = 45
    self.spacerCutoutWidth = 2.5
    
    self.m3ScrewHeadDiameter = 6.0 

    design = inksnek.add_group(inksnek.top_group, inksnek.translate_group(inksnek.template_width/2.0, 15.0))
    p2Right = inksnek.add_group(design, inksnek.translate_group(self.plate1WidthBase/2 + self.xplode, 0.0))
    p2Left = inksnek.add_group(design, inksnek.scale_group(-1.0, +1.0) + inksnek.translate_group(self.plate1WidthBase/2 + self.xplode, 0.0))
    plate3dY = 0.0 if self.render else self.plate2Radius + 1.0
    p3Right = inksnek.add_group(p2Right, inksnek.translate_group(self.xplode, plate3dY))
    p3Left = inksnek.add_group(p2Left, inksnek.translate_group(self.xplode, plate3dY))
    self.addPlate1(design)
    self.addPlate2(p2Right, self.plate1WidthBase/2)
    self.addPlate2(p2Left, self.plate1WidthBase/2)
    self.addPlate3(p3Right, self.plate1WidthBase/2, True)
    self.addPlate3(p3Left, self.plate1WidthBase/2, False)
    innerBoardPlate = design if self.render else inksnek.add_group(design, inksnek.translate_group(0.0, self.plate1Height + self.plate1Peak + 1.0))
    self.addBoardPlate(innerBoardPlate, False)
    outerBoardPlate = design if self.render else inksnek.add_group(design, inksnek.translate_group(0.0, self.plate1Height + 2*(self.plate1Peak + 1.0) + self.plate2Radius))
    self.addBoardPlate(outerBoardPlate, True)
    
    if not self.render:
        psRight = inksnek.add_group(innerBoardPlate, inksnek.translate_group(self.plate1WidthBase/2 + self.plateOverlap + 2.0, 0))
        self.addPlateSpacer(psRight, self.plate1WidthBase/2)
        psLeft = inksnek.add_group(innerBoardPlate, inksnek.scale_group(-1.0, +1.0) + inksnek.translate_group(self.plate1WidthBase/2 + self.plateOverlap + 2.0, 0))
        self.addPlateSpacer(psLeft, self.plate1WidthBase/2)

    
if __name__ == '__main__':
    e = my_design()
    e.affect()
