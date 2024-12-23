// Renders of laser-cut enclosure for "Falling Water"
// TroGlass Sky Blue 3mm?
// todo
// shallow 'v' on top edge, or '^'?
// etched line along outisde
// vertically position LCD to clamp? leaving space for wire. Support at back to stand?
materialThickness = 3.0;

// the gap the LCD sticks throught

lcdBoardWidth = 24;
lcdBoardHeight = 92;
lcdBoardThickness = 1.5;
lcdDisplayWidth = 19.5;
lcdDisplayHeight = 78.5;
lcdDisplayBottomOffset = 5.0; // bottom of board to bottom of LCD glass layer
lcdDisplayThickness = 6; // ignoring extra bit of backlight at the top

lcdVoidWidth = lcdDisplayWidth + 1.0;
lcdVoidHeight = lcdDisplayHeight + 1.0;


plateOverlap = 5;
golden = 1.61803399;
xplode = 0;
epsilon = 0.01; // merge plates?

plate1Height = 110.0;
plate1WidthBase = 40.0;
plate1WidthTop = golden*plate1WidthBase;//80.0;
plate1Angle = atan2(plate1Height, (plate1WidthTop - plate1WidthBase)/2.0);
plate1Radius = sqrt(plate1Height^2 + (plate1WidthTop-plate1WidthBase)^2);

plate2Angle = 2*plate1Angle/3.0;
plate2Radius = plate1Radius/golden;//3*plate1Radius/4;

plate3Angle = 0.0;
plate3Radius = plate2Radius/golden;//3*plate2Radius/4;

plateHoleDiameter = 2.5;

module LCD()
{
  // lcd, origin is bottom left of glass
  alpha = 1.0;
  translate([-(lcdBoardWidth-lcdDisplayWidth)/2, -lcdDisplayBottomOffset, -(lcdBoardThickness + lcdDisplayThickness/2)])
  {
    color("green", alpha)
      cube([lcdBoardWidth, lcdBoardHeight, lcdBoardThickness]);
    color("aqua", alpha)
      translate([(lcdBoardWidth-lcdDisplayWidth)/2, lcdDisplayBottomOffset,lcdBoardThickness])
        cube([lcdDisplayWidth, lcdDisplayHeight, lcdDisplayThickness]);
  }
}

module holes()
{
  $fn=20;
  translate([plate1WidthBase/2 - plateOverlap/2, plateOverlap/2, -epsilon])
    cylinder(h=4.0*(materialThickness + epsilon)+ epsilon, d=plateHoleDiameter, center=true);
  translate([plate1WidthBase/2 + plateOverlap/2 + plate2Radius*cos(plate1Angle) - plateOverlap, plate2Radius*sin(plate1Angle) - plateOverlap/2, -epsilon])
    cylinder(h=4.0*(materialThickness + epsilon)+ epsilon, d=plateHoleDiameter, center=true);
}

module plate1()
{
  // origin is middle of bottom edge
  lcdVoidPoints = 
  [
    [-lcdVoidWidth/2.0, 0.0],
    [+lcdVoidWidth/2.0, 0.0], 
    [+lcdVoidWidth/2.0, lcdVoidHeight],
    [-lcdVoidWidth/2.0, lcdVoidHeight]
  ];
  plate1Points = 
  [
    [-plate1WidthBase/2.0, 0.0],
    [+plate1WidthBase/2.0, 0.0], 
    [+plate1WidthTop/2.0, plate1Height],
      [0.0, plate1Height + 5],  // paek/valley todo, half the plate2 slope?
    [-plate1WidthTop/2.0, plate1Height]
  ];
  difference()
  {
    linear_extrude(materialThickness)
      polygon(plate1Points);
    translate([0.0, (plate1Height-lcdVoidHeight)/2.0, -epsilon/2])
      linear_extrude(materialThickness + epsilon)
        polygon(lcdVoidPoints);
  }
}

module plate2()
{
  // right hand. origin is bottom corner (excluding overlap), anti-clockwise
  points = 
  [
    [-plateOverlap, 0],
    [0, 0],
    [plate2Radius*cos(plate2Angle), plate2Radius*sin(plate2Angle)],
    [plate2Radius*cos(plate1Angle), plate2Radius*sin(plate1Angle)],   
    [plate2Radius*cos(plate1Angle) - plateOverlap, plate2Radius*sin(plate1Angle)]
  ];
  linear_extrude(materialThickness + epsilon)
    polygon(points);  
}

module plate3()
{
  // right hand. origin is bottom corner (excluding overlap)
  if (true) 
  {
    // extend to top corner of plate2? common hole
    points = 
    [
      [-plateOverlap, 0],
      [plate3Radius*cos(plate3Angle), plate3Radius*sin(plate3Angle)],
        [plate2Radius*cos(plate1Angle), plate2Radius*sin(plate1Angle)],   
        [plate2Radius*cos(plate1Angle) - plateOverlap, plate2Radius*sin(plate1Angle)]
    ];
    linear_extrude(materialThickness + epsilon)
      polygon(points);    
  }
  else
  {
    points = 
    [
      [-plateOverlap, 0],
      [plate3Radius*cos(plate3Angle), plate3Radius*sin(plate3Angle)],
      [plate3Radius*cos(plate2Angle), plate3Radius*sin(plate2Angle)],   
      [plate3Radius*cos(plate2Angle) - plateOverlap, plate3Radius*sin(plate2Angle)],
      [-plateOverlap, plateOverlap]
    ];
    linear_extrude(materialThickness + epsilon)
      polygon(points);    
  }
}

// MAIN
difference()
{
  union()
  {
    color("blue")
    {
      plate1();
      translate([plate1WidthBase/2 + xplode, 0, -materialThickness])
      {
        plate2();
        translate([xplode, 0, -materialThickness])
          plate3();
      }
      mirror([1,0,0])
      {
        translate([plate1WidthBase/2 + xplode, 0, -materialThickness])
        {
          plate2();
          translate([xplode, 0, -materialThickness])
            plate3();
        }
      }
    }
  }
  holes();
  mirror([1,0,0])
    holes();
}
translate([-lcdDisplayWidth/2, (plate1Height-lcdDisplayHeight)/2.0, 0])
  LCD();
