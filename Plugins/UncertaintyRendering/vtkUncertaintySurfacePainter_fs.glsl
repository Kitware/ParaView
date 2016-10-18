#version 120

varying vec4 vColor;
varying vec3 v_texCoord3D;
varying float vUncertainty;
uniform sampler2D texture;
uniform float uncertaintyScaleFactor;
uniform float scalarValueRange;
uniform sampler2D permTexture;
uniform sampler1D simplexTexture;
uniform sampler2D gradTexture;

/*
 * To create offsets of one texel and one half texel in the
 * texture lookup, we need to know the texture image size.
 */
#define ONE 0.00390625
#define ONEHALF 0.001953125
// The numbers above are 1/256 and 0.5/256, change accordingly
// if you change the code to use another texture size.

/*
 * 4D simplex noise. A lot faster than classic 4D noise, and better looking.
 */
float snoise(vec4 P)
{

// The skewing and unskewing factors are hairy again for the 4D case
// This is (sqrt(5.0)-1.0)/4.0
#define F4 0.309016994375
// This is (5.0-sqrt(5.0))/20.0
#define G4 0.138196601125

  // Skew the (x,y,z,w) space to determine which cell of 24 simplices we're in
  float s = (P.x + P.y + P.z + P.w) * F4; // Factor for 4D skewing
  vec4 Pi = floor(P + s);
  float t = (Pi.x + Pi.y + Pi.z + Pi.w) * G4;
  vec4 P0 = Pi - t;        // Unskew the cell origin back to (x,y,z,w) space
  Pi = Pi * ONE + ONEHALF; // Integer part, scaled and offset for texture lookup

  vec4 Pf0 = P - P0; // The x,y distances from the cell origin

  // For the 4D case, the simplex is a 4D shape I won't even try to describe.
  // To find out which of the 24 possible simplices we're in, we need to
  // determine the magnitude ordering of x, y, z and w components of Pf0.
  // The method below is presented without explanation. It uses a small 1D
  // texture as a lookup table. The table is designed to work for both
  // 3D and 4D noise and contains 64 indices, of which only 24 are actually
  // used. An extension to 5D would require a larger texture here.
  float c1 = (Pf0.x > Pf0.y) ? 0.5078125 : 0.0078125; // 1/2 + 1/128
  float c2 = (Pf0.x > Pf0.z) ? 0.25 : 0.0;
  float c3 = (Pf0.y > Pf0.z) ? 0.125 : 0.0;
  float c4 = (Pf0.x > Pf0.w) ? 0.0625 : 0.0;
  float c5 = (Pf0.y > Pf0.w) ? 0.03125 : 0.0;
  float c6 = (Pf0.z > Pf0.w) ? 0.015625 : 0.0;
  float sindex = c1 + c2 + c3 + c4 + c5 + c6;
  vec4 offsets = texture1D(simplexTexture, sindex).rgba;
  vec4 o1 = step(0.625, offsets);
  vec4 o2 = step(0.375, offsets);
  vec4 o3 = step(0.125, offsets);

  // Noise contribution from simplex origin
  float perm0xy = texture2D(permTexture, Pi.xy).a;
  float perm0zw = texture2D(permTexture, Pi.zw).a;
  vec4 grad0 = texture2D(gradTexture, vec2(perm0xy, perm0zw)).rgba * 4.0 - 1.0;
  float t0 = 0.6 - dot(Pf0, Pf0);
  float n0;
  if (t0 < 0.0)
    n0 = 0.0;
  else
  {
    t0 *= t0;
    n0 = t0 * t0 * dot(grad0, Pf0);
  }

  // Noise contribution from second corner
  vec4 Pf1 = Pf0 - o1 + G4;
  o1 = o1 * ONE;
  float perm1xy = texture2D(permTexture, Pi.xy + o1.xy).a;
  float perm1zw = texture2D(permTexture, Pi.zw + o1.zw).a;
  vec4 grad1 = texture2D(gradTexture, vec2(perm1xy, perm1zw)).rgba * 4.0 - 1.0;
  float t1 = 0.6 - dot(Pf1, Pf1);
  float n1;
  if (t1 < 0.0)
    n1 = 0.0;
  else
  {
    t1 *= t1;
    n1 = t1 * t1 * dot(grad1, Pf1);
  }

  // Noise contribution from third corner
  vec4 Pf2 = Pf0 - o2 + 2.0 * G4;
  o2 = o2 * ONE;
  float perm2xy = texture2D(permTexture, Pi.xy + o2.xy).a;
  float perm2zw = texture2D(permTexture, Pi.zw + o2.zw).a;
  vec4 grad2 = texture2D(gradTexture, vec2(perm2xy, perm2zw)).rgba * 4.0 - 1.0;
  float t2 = 0.6 - dot(Pf2, Pf2);
  float n2;
  if (t2 < 0.0)
    n2 = 0.0;
  else
  {
    t2 *= t2;
    n2 = t2 * t2 * dot(grad2, Pf2);
  }

  // Noise contribution from fourth corner
  vec4 Pf3 = Pf0 - o3 + 3.0 * G4;
  o3 = o3 * ONE;
  float perm3xy = texture2D(permTexture, Pi.xy + o3.xy).a;
  float perm3zw = texture2D(permTexture, Pi.zw + o3.zw).a;
  vec4 grad3 = texture2D(gradTexture, vec2(perm3xy, perm3zw)).rgba * 4.0 - 1.0;
  float t3 = 0.6 - dot(Pf3, Pf3);
  float n3;
  if (t3 < 0.0)
    n3 = 0.0;
  else
  {
    t3 *= t3;
    n3 = t3 * t3 * dot(grad3, Pf3);
  }

  // Noise contribution from last corner
  vec4 Pf4 = Pf0 - vec4(1.0 - 4.0 * G4);
  float perm4xy = texture2D(permTexture, Pi.xy + vec2(ONE, ONE)).a;
  float perm4zw = texture2D(permTexture, Pi.zw + vec2(ONE, ONE)).a;
  vec4 grad4 = texture2D(gradTexture, vec2(perm4xy, perm4zw)).rgba * 4.0 - 1.0;
  float t4 = 0.6 - dot(Pf4, Pf4);
  float n4;
  if (t4 < 0.0)
    n4 = 0.0;
  else
  {
    t4 *= t4;
    n4 = t4 * t4 * dot(grad4, Pf4);
  }

  // Sum up and scale the result to cover the range [-1,1]
  return 27.0 * (n0 + n1 + n2 + n3 + n4);
}

vec2 mix_random_noise(vec2 inputCoord, float uncertainty)
{
  float n = 0.0;
  float s = uncertaintyScaleFactor;
  int octs = 5;
  for (int i = 0; i < octs; i++)
  {
    n += snoise(vec4(4.0 * pow(2.0, float(i)) * s * v_texCoord3D.xyz, gl_TexCoord[0].x));
  }
  n /= float(octs);

  float scale = scalarValueRange;

  inputCoord.x = clamp(inputCoord.x + uncertaintyScaleFactor * uncertainty / scale * n, 0.0f, 1.0f);
  return inputCoord;
}

void main()
{
  // offset the texture coordinate by the uncertainty value
  float uncertainty = vUncertainty;
  vec2 coord = mix_random_noise(gl_TexCoord[0].xy, uncertainty);
  gl_FragColor = vColor * texture2D(texture, coord);
}
