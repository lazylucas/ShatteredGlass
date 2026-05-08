#pragma once

// ─── Vertex shader ────────────────────────────────────────────────────────────
static const char* kVertSrc = R"GLSL(
#version 330 core
layout(location = 0) in vec2 inPos;
layout(location = 1) in vec2 inUV;
out vec2 vUV;
void main() {
    vUV = inUV;
    gl_Position = vec4(inPos, 0.0, 1.0);
}
)GLSL";

// ─── Fragment shader (Physically-Based Shattered Glass v2) ───────────────────
static const char* kFragSrc = R"GLSL(
#version 330 core

// ════════════════════════════════════════════════════════════════════════════
//  PHYSICALLY-BASED SHATTERED GLASS  v2
//
//  Realism improvements over v1:
//   1. STRAIGHT-LINE cracks: point-to-segment distance replaces Voronoi curves
//      Real glass fractures along minimum-energy tensile paths = straight lines
//   2. CRACK BRANCHING: each primary crack spawns 1-2 secondaries
//   3. PER-SHARD TILT: each Voronoi cell gets a random tilt normal
//      (glass shards physically rotate as they separate)
//   4. FRESNEL EDGE: specular rim + black void — not flat tinted line
//   5. PROPER REFRACTION: per-shard tilt drives the UV offset direction
//      Chromatic split is radial from the tilt axis, not horizontal
//   6. SPALL ZONE: opaque crushed-glass cloud at impact epicentre
//   7. RADIAL DENSITY FALLOFF: far shards are large (fewer, wider cracks)
//   8. SHARD FACE MICRO-TEXTURE: conchoidal fracture noise on surface
//   9. PROPAGATION RADIUS: cracks stop at a distance tied to impactForce
// ════════════════════════════════════════════════════════════════════════════
const FRAG_SRC = `#version 330 core
in  vec2 vUV;
out vec4 fragColor;

uniform sampler2D inputTexture;
uniform vec2  texSize;
uniform float time;

uniform vec2  impact;
uniform float impactForce;
uniform float crackSpread;
uniform float radialCracks;
uniform float stressRings;
uniform float refractionDepth;
uniform float shardSeparation;
uniform float edgeLight;
uniform float chromatic;
uniform float dirt;
uniform float bloom;
uniform float animSpeed;
uniform float seed;
uniform float freeze;

#define PI  3.14159265359
#define TAU 6.28318530718

// ─── Hashing ──────────────────────────────────────────────────────────────────
float hash11(float n) { n=fract(n*.1031); n*=n+33.33; n*=n+n; return fract(n); }
float hash21(vec2 p) {
    vec3 p3=fract(vec3(p.xyx)*.1031);
    p3+=dot(p3,p3.yzx+33.33);
    return fract((p3.x+p3.y)*p3.z);
}
vec2 hash22(vec2 p) {
    vec3 p3=fract(vec3(p.xyx)*vec3(.1031,.1030,.0973));
    p3+=dot(p3,p3.yzx+33.33);
    return fract((p3.xx+p3.yz)*p3.zy);
}
vec3 hash31(float n) {
    vec3 p=fract(vec3(n)*vec3(.1031,.1030,.0973));
    p+=dot(p,p.yzx+33.33);
    return fract((p.xxy+p.yzz)*p.zyx);
}

float vnoise(vec2 p) {
    vec2 i=floor(p), f=fract(p);
    vec2 u=f*f*(3.0-2.0*f);
    return mix(mix(hash21(i),hash21(i+vec2(1,0)),u.x),
               mix(hash21(i+vec2(0,1)),hash21(i+vec2(1,1)),u.x),u.y);
}

// ─── Point-to-segment distance ────────────────────────────────────────────────
float segDist(vec2 p, vec2 a, vec2 b) {
    vec2 ab=b-a, ap=p-a;
    float t=clamp(dot(ap,ab)/dot(ab,ab),0.0,1.0);
    return length(ap-ab*t);
}

// ─── Voronoi (for shard IDs and tilt, NOT for crack borders) ─────────────────
//  Returns: .x = dist to closest centre, .yz = closest cell coord, .w = cellID
vec4 voronoiID(vec2 p) {
    vec2 i=floor(p), f=fract(p);
    float d1=8.0;
    vec2 closest=vec2(0.0);
    vec2 cellOrigin=vec2(0.0);
    for (int j=-2;j<=2;j++) for (int k=-2;k<=2;k++) {
        vec2 g=vec2(float(k),float(j));
        vec2 o=hash22((i+g)+seed*7.31);
        vec2 r=g+o-f;
        float d=dot(r,r);
        if(d<d1){d1=d; closest=r; cellOrigin=i+g;}
    }
    return vec4(sqrt(d1), closest, hash21(cellOrigin+seed*19.7));
}

// ─── PRIMARY + BRANCHING CRACKS ───────────────────────────────────────────────
//  Each crack is a polyline: origin → mid-point (slightly deflected) → tip.
//  Branching: secondaries spawn at 60-65% along the primary.

float crackNetwork(vec2 uv, vec2 imp, float intensity, float propRadius) {
    float totalCrack = 0.0;
    float numPrimary = floor(mix(3.0, 20.0, intensity));

    for (float k=0.0; k<20.0; k+=1.0) {
        if (k >= numPrimary) break;

        // Primary crack angle + random spread
        float baseAng  = (k/numPrimary)*TAU + hash11(k+seed*13.0+1.0)*(PI*0.35);
        float len      = propRadius * mix(0.55, 1.05, hash11(k+seed*31.0));

        // Primary: two segments with a slight bend at midpoint
        vec2 tip     = imp + vec2(cos(baseAng),sin(baseAng))*len;
        float bend   = (hash11(k+seed*7.7)-0.5)*0.12;
        vec2  mid    = mix(imp,tip,0.5) + vec2(-sin(baseAng),cos(baseAng))*bend*len;

        // Crack width tapers: wide near impact, hairline at tip
        //  We measure distance along the crack for taper
        float d1     = segDist(uv,imp,mid);
        float d2     = segDist(uv,mid,tip);
        float dCrack = min(d1,d2);

        // Taper: t=0 at impact end, 1 at tip
        float taper  = smoothstep(0.0,len,distance(uv,imp));
        float crackW = mix(0.004, 0.0008, taper) * (1.0 + impactForce*1.5);
        float crack  = 1.0-smoothstep(0.0, crackW, dCrack);

        // Only within propagation radius
        float distFromImp = distance(uv, imp);
        crack *= step(distFromImp, len*1.05);
        totalCrack = max(totalCrack, crack);

        // ── Branch 1 at ~55% along primary ──────────────────────────────────
        float branchT    = 0.55 + hash11(k*3.1+seed)*0.15;
        vec2  branchOrig = mix(imp, mid, branchT*2.0); // lerp on first half
        float branchAng  = baseAng + (hash11(k*5.3+seed)-0.5)*PI*0.7 + PI*0.15;
        float branchLen  = len * mix(0.25, 0.55, hash11(k*2.7+seed));
        vec2  branchTip  = branchOrig + vec2(cos(branchAng),sin(branchAng))*branchLen;
        float branchBend = (hash11(k*9.1+seed)-0.5)*0.15;
        vec2  branchMid  = mix(branchOrig,branchTip,0.5)
                         + vec2(-sin(branchAng),cos(branchAng))*branchBend*branchLen;
        float db1        = segDist(uv,branchOrig,branchMid);
        float db2        = segDist(uv,branchMid,branchTip);
        float dBranch    = min(db1,db2);
        float taperB     = smoothstep(0.0,branchLen,distance(uv,branchOrig));
        float branchW    = mix(0.0025,0.0005,taperB)*(1.0+impactForce*1.2);
        float brCrack    = 1.0-smoothstep(0.0,branchW,dBranch);
        brCrack         *= step(distance(uv,branchOrig), branchLen*1.05);
        totalCrack       = max(totalCrack, brCrack*0.9);

        // ── Branch 2 (other side) if crack dense enough ──────────────────────
        if (intensity > 0.5) {
            float b2Ang  = baseAng - (hash11(k*4.9+seed+2.0))*PI*0.6 - PI*0.12;
            float b2Len  = len * mix(0.2,0.45,hash11(k*3.3+seed+1.0));
            float b2T    = 0.65 + hash11(k*6.7+seed)*0.2;
            vec2  b2Orig = mix(imp,tip,b2T);
            vec2  b2Tip  = b2Orig + vec2(cos(b2Ang),sin(b2Ang))*b2Len;
            float db2c   = segDist(uv,b2Orig,b2Tip);
            float taperB2= smoothstep(0.0,b2Len,distance(uv,b2Orig));
            float bW2    = mix(0.002,0.0004,taperB2)*(1.0+impactForce);
            float b2Crack= 1.0-smoothstep(0.0,bW2,db2c);
            b2Crack     *= step(distance(uv,b2Orig),b2Len*1.05);
            totalCrack   = max(totalCrack, b2Crack*0.8);
        }
    }
    return clamp(totalCrack, 0.0, 1.0);
}

// ─── STRESS RINGS (kept as concentric arcs — that part was realistic) ─────────
float stressRing(vec2 uv, vec2 imp, float intensity, float t, float propRadius) {
    if (intensity<0.001) return 0.0;
    float dist    = length(uv-imp);
    float numRings= floor(mix(0.0,6.0,intensity));
    if (numRings<0.5) return 0.0;
    float ring=0.0;
    for (float k=1.0;k<=6.0;k+=1.0) {
        if (k>numRings) break;
        float ringRadius = (k/6.0)*propRadius*0.9 + hash11(k+seed*11.0)*0.03;
        ringRadius += t*0.015;
        // Rings get thinner further out (less stress there)
        float ringWidth = (0.003+hash11(k*2.7)*0.003) * (1.0-k/8.0);
        float band = 1.0-smoothstep(0.0,ringWidth,abs(dist-ringRadius));
        // Arcs — not full circles (real rings always break)
        float ang    = atan(uv.y-imp.y, uv.x-imp.x);
        float angMask= step(0.35, vnoise(vec2(ang*3.0,k*5.7+seed)));
        // Fade rings beyond propagation radius
        float radFade= 1.0-smoothstep(propRadius*0.8,propRadius,dist);
        ring = max(ring, band*angMask*radFade);
    }
    return ring;
}

// ─── VORONOI CRACKS (keeps shard structure, but narrow — now secondary) ────────
float voronoiCracks(vec2 p, float density) {
    vec2 i=floor(p), f=fract(p);
    float d1=8.0,d2=8.0;
    for (int j=-1;j<=1;j++) for (int k=-1;k<=1;k++) {
        vec2 g=vec2(float(k),float(j));
        vec2 o=hash22((i+g)+seed*7.31);
        vec2 r=g+o-f;
        float d=dot(r,r);
        if(d<d1){d2=d1;d1=d;}else if(d<d2){d2=d;}
    }
    // Very thin Voronoi border = shard surface micro-fractures
    float border=sqrt(d2)-sqrt(d1);
    return 1.0-smoothstep(0.0,0.018,border);
}

// ─── FRESNEL / EDGE SPECULAR ─────────────────────────────────────────────────
//  Physically: crack void is black; rim catches specular.
//  We need an inner (dark) + outer (bright) band.
//  crackMask is 1 on crack, 0 in shard. We use raw crackDist.
vec3 fresnelEdge(vec3 col, float crackMask, float crackDist, vec3 lightDir) {
    // Void: near-black core of crack
    float voidMask = smoothstep(0.5, 1.0, crackMask);
    col = mix(col, vec3(0.02, 0.02, 0.025), voidMask*0.95);

    // Specular rim just outside the void — cool white with a hint of blue
    // (this is refracted skylight — glass edges are always slightly blue-white)
    float rimMask = smoothstep(0.0,0.6,crackMask) * (1.0-smoothstep(0.3,0.8,crackMask));
    vec3  rimCol  = vec3(0.92, 0.96, 1.0) * (1.2 + lightDir.z*0.5);
    col = mix(col, rimCol, rimMask * edgeLight * 0.9);

    return col;
}

// ─── PER-SHARD TILT + REFRACTION ─────────────────────────────────────────────
//  Each shard has a random tilt normal. The tilt drives UV offset (refraction)
//  and modulates the face brightness (like a Phong normal map).
void shardTilt(float cellID, float cellID2,
               out vec2 tiltUV, out float faceBright) {
    // Two independent random angles → tilt vector
    float tiltMag = refractionDepth * 0.04 * impactForce;
    float angA    = hash11(cellID  * 17.3 + seed*3.1) * TAU;
    float angB    = hash11(cellID2 * 23.7 + seed*5.9) * TAU;
    vec2  tilt    = vec2(cos(angA), sin(angB)) * tiltMag;

    // Refraction UV offset = tilt direction
    tiltUV = tilt;

    // Face brightness: dot(tiltNormal, lightDir) — fake Phong
    vec3 tiltNorm = normalize(vec3(tilt*8.0, 1.0));
    vec3 lightDir = normalize(vec3(0.4, 0.7, 1.0)); // fixed directional
    faceBright    = 0.75 + 0.35 * max(0.0, dot(tiltNorm, lightDir));
}

// ─── CHROMATIC ABERRATION (radial from tilt axis, not horizontal) ─────────────
vec3 chromaticSample(vec2 uv, vec2 tiltDir, float crackProx, float chromaAmt) {
    // Dispersion is strongest at crack edges, zero on flat surface
    float ca = chromaAmt * crackProx * 0.018 * impactForce;
    // Direction is perpendicular to the tilt axis (dispersion axis of a tilted prism)
    vec2 dispDir = length(tiltDir) > 0.0001 ?
                   normalize(vec2(-tiltDir.y, tiltDir.x)) : vec2(1.0,0.0);
    float r = texture(inputTexture, uv + dispDir*ca*1.5).r;
    float g = texture(inputTexture, uv).g;
    float b = texture(inputTexture, uv - dispDir*ca*1.5).b;
    return vec3(r,g,b);
}

// ─── SPALL ZONE (pulverised glass cloud at impact epicentre) ──────────────────
vec3 spallZone(vec3 col, vec2 uv, vec2 imp, float force) {
    float dist   = length(uv-imp);
    float radius = 0.02 + force*0.04;
    float spall  = 1.0-smoothstep(0.0, radius, dist);
    // Opaque white-grey powdered glass
    vec3  spallCol = vec3(0.88, 0.90, 0.92);
    // Add some conchoidal texture
    spallCol += (vnoise(uv*400.0+seed)-0.5)*0.08;
    return mix(col, spallCol, spall*force*0.9);
}

// ─── CONCHOIDAL MICRO-TEXTURE on shard faces ──────────────────────────────────
float conchoidal(vec2 uv, float cellID) {
    // Conchoidal = ripple-like fracture patterns on each shard face
    // Oriented differently per shard (cellID rotates the noise)
    float ang = cellID * TAU;
    float c=cos(ang), s=sin(ang);
    vec2 rotUV = vec2(uv.x*c-uv.y*s, uv.x*s+uv.y*c);
    float n  = vnoise(rotUV*180.0)*0.5
             + vnoise(rotUV*440.0)*0.3
             + vnoise(rotUV*900.0)*0.2;
    return n;
}

// ─── DIRT / SCRATCHES ─────────────────────────────────────────────────────────
float dirtMap(vec2 uv, float cellID) {
    // Scratches oriented per-shard (in-plane with the shard)
    float shardAng = cellID * TAU;
    float c=cos(shardAng), s=sin(shardAng);
    vec2 ru = vec2(uv.x*c-uv.y*s, uv.x*s+uv.y*c);
    float scratch = 0.0;
    for (float i=0.0;i<5.0;i+=1.0) {
        float a   = hash11(i+cellID*3.1+seed*5.7)*PI;
        vec2  dir = vec2(cos(a),sin(a));
        float s1  = abs(dot(ru-0.5, vec2(-dir.y,dir.x)) + hash11(i*3.3+seed)*0.3);
        scratch   = max(scratch, (1.0-smoothstep(0.0,0.0012,s1))*(0.3+hash11(i*7.1)*0.5));
    }
    float grain = vnoise(uv*350.0)*0.5 + vnoise(uv*900.0)*0.3 + vnoise(uv*2100.0)*0.2;
    return clamp(grain*0.35 + scratch*0.65, 0.0, 1.0);
}

// ════════════════════════════════════════════════════════════════════════════
//  MAIN
// ════════════════════════════════════════════════════════════════════════════
void main() {
    vec2 uv  = vUV;
    vec2 imp = impact;
    float t  = mix(time*animSpeed, 0.0, freeze);

    // ── Propagation radius: scales with impactForce ──────────────────────────
    float propRadius = mix(0.15, 0.85, impactForce);

    // ── Distance from impact (normalised by propRadius) ──────────────────────
    float impDist    = length(uv-imp);
    float distNorm   = impDist / max(propRadius, 0.001);

    // ── Voronoi shard IDs ────────────────────────────────────────────────────
    // Density is high near impact, low far away (inverse distance)
    float densityNear = mix(55.0, 10.0, crackSpread);
    float densityFar  = densityNear * 0.25;
    float density     = mix(densityNear, densityFar, smoothstep(0.0, 1.0, distNorm));

    vec4  vor     = voronoiID(uv * density);
    float cellID  = vor.w;
    float cellID2 = hash11(cellID*31.7 + 0.37);  // second independent cell random

    // ── Crack network (straight lines + branches) ────────────────────────────
    float primary = crackNetwork(uv, imp, radialCracks, propRadius);

    // ── Thin Voronoi secondary cracks (shard internal micro-fractures) ───────
    float secondary = voronoiCracks(uv*density, density) * 0.5
                    * smoothstep(propRadius, 0.0, impDist)  // only inside prop radius
                    * impactForce;

    // ── Stress rings ─────────────────────────────────────────────────────────
    float rings = stressRing(uv, imp, stressRings, t, propRadius);

    // ── Combined crack mask ──────────────────────────────────────────────────
    float crackMask = clamp(max(max(primary, secondary), rings), 0.0, 1.0);

    // Raw un-clamped crack distance for Fresnel calculation
    float crackProximity = max(max(primary, secondary*0.6), rings);

    // ── Fade everything outside propagation radius ───────────────────────────
    float radialFade = 1.0 - smoothstep(propRadius*0.85, propRadius, impDist);
    crackMask  *= radialFade;
    crackProximity *= radialFade;

    // ── Per-shard tilt → refraction UV + face brightness ────────────────────
    vec2  tiltUV   = vec2(0.0);
    float faceBright = 1.0;
    shardTilt(cellID, cellID2, tiltUV, faceBright);

    // Shard separation: push shards away from impact
    vec2 shardOff = vec2(0.0);
    if (shardSeparation > 0.001) {
        vec2 awayDir = normalize(uv - imp + vec2(0.0001));
        float mag    = (cellID-0.5)*2.0 * shardSeparation*0.035;
        mag         *= smoothstep(0.6, 0.0, distNorm) * impactForce;
        shardOff     = awayDir * mag;
    }

    // Total UV offset = tilt refraction + shard separation
    vec2 sampleUV = uv + tiltUV * (1.0 - crackMask) + shardOff;
    sampleUV = clamp(sampleUV, 0.001, 0.999);

    // ── Sample with chromatic dispersion ─────────────────────────────────────
    vec3 col = chromaticSample(sampleUV, tiltUV, crackProximity, chromatic);

    // ── Shard face shading (physically-based tilt Phong) ─────────────────────
    // Only on surfaces, not on cracks
    float surfaceMask = 1.0 - smoothstep(0.0, 0.5, crackMask);
    col *= mix(1.0, faceBright, surfaceMask * 0.6);

    // Conchoidal micro-texture on shard faces (subtle)
    float conch = conchoidal(uv, cellID);
    col *= mix(1.0, 0.9 + conch*0.15, surfaceMask*0.25*impactForce);

    // ── Fresnel edge: void + specular rim ────────────────────────────────────
    vec3 lightDir = normalize(vec3(0.4, 0.7, 1.0));
    col = fresnelEdge(col, crackMask, crackProximity, lightDir);

    // ── Bloom (light scatter around bright rim) ───────────────────────────────
    if (bloom > 0.001) {
        // Gaussian 3-tap bloom from shard edges
        vec2 bDir = normalize(tiltUV + vec2(0.001)) * 0.003 * bloom;
        vec3 b0   = texture(inputTexture, sampleUV + bDir).rgb;
        vec3 b1   = texture(inputTexture, sampleUV - bDir).rgb;
        vec3 b2   = texture(inputTexture, sampleUV + bDir.yx).rgb;
        vec3 bloomCol = (b0+b1+b2)/3.0;
        col += bloomCol * crackMask * bloom * 0.25;
    }

    // ── Dirt / scratches on shard faces ──────────────────────────────────────
    if (dirt > 0.001) {
        float d       = dirtMap(uv, cellID);
        // Dirt in crack voids (dust settling in cracks)
        float voidDirt  = smoothstep(0.4,1.0,crackMask) * d * dirt * 0.7;
        col = mix(col, vec3(0.15,0.14,0.12), voidDirt);
        // Scratches on shard surfaces
        float faceDirt  = surfaceMask * d * dirt * 0.3;
        col = mix(col, col*0.85 + vec3(0.85,0.87,0.9)*0.12, faceDirt);
    }

    // ── Spall zone (crushed glass at epicentre) ───────────────────────────────
    col = spallZone(col, uv, imp, impactForce);

    // ── Tone + gamma ─────────────────────────────────────────────────────────
    col = clamp(col, 0.0, 1.0);
    // Subtle contrast lift so cracks pop without blowing out
    col = col * (1.0 + col*0.12);
    col = clamp(col, 0.0, 1.0);

    fragColor = vec4(col, 1.0);

)GLSL";
