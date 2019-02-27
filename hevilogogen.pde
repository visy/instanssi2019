PGraphics g;
PGraphics g2;

int gw = 160;
int gh = 100;

int g2w = 320;
int g2h = 400;

void setup() {
  size(320,400);
  g = createGraphics(gw,gh);
  g2 = createGraphics(g2w,g2h);
  frameRate(1);
}

int nSeed = 5323;

int prng() {
    nSeed = (8253729 * nSeed + 2396403); 
    return nSeed  % 32767;
}

void draw() {
  background(128,128,192);
  g2.beginDraw();
  g2.background(0,0,0);
  g.beginDraw();
  g.background(0);
  g.loadPixels();
  int r = prng();
  for (int y = 0; y < gh; y++) {
    for (int x = 0; x < gw; x+=1) {
      float pe = 0.05;
      int col = (int)((0xFF*cos(r+y*pe+tan(x*100.01))*sin((x*abs(cos(r+300))-40)*pe)*tan(x*0.01+cos(x*r*0.01+y*0.1)*r*0.00009*cos(tan(x*0.2+r)))));
      int dd = abs(gh/2-y);
      if (dd > 20) col -= (dd-20)*0x50;
      if (col < 0xFF) g.pixels[y*gw+x] = 0x00000000;
      else g.pixels[y*gw+x] = 0xFFFFFFFF;

      //if (col > 0xFF) g.pixels[(y*gw)+(x+1)] = 0xAAAAAAAA;
  
    }
  }
  for (int y = 0; y < gh; y++) {
    for (int x = 0; x < gw/2; x++) {
      g.pixels[y*gw+x] = g.pixels[(y*gw)+(gw-1-x)];
    }
  }

  g.updatePixels();
  g.endDraw();
  //g2.image(g,0,0,g2w,g2h);
  g2.image(g,g2w/2-gw/2,g2h/2-gh/2);
  g2.endDraw();
  image(g2,0,0,width,height);
  
}
