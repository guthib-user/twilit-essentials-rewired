// Native adaptations of Comic.fx (Alexander Federwisch, BSD-3-Clause)
// and SweetFX (CeeJay.dk and contributors, MIT). See res/SOURCES.md.
// No bloom/blur/grain pass is implemented. The source image is already lit
// and shadowed by Dusklight/Essentials. HUD is drawn after this pass.
struct Uniforms { settings: vec4f, camera: vec4f }
@group(0) @binding(0) var scene: texture_2d<f32>;
@group(0) @binding(1) var depth: texture_2d<f32>;
@group(0) @binding(2) var<uniform> u: Uniforms;

@vertex fn vertex_main(@builtin(vertex_index) i:u32) -> @builtin(position) vec4f {
    let x=f32((i<<1u)&2u); let y=f32(i&2u);
    return vec4f(x*2.0-1.0,1.0-y*2.0,0.0,1.0);
}
fn sat(c:vec3f)->vec3f{return clamp(c,vec3f(0),vec3f(1));}
fn hi(c:vec3f)->f32{return max(c.r,max(c.g,c.b));}
fn lo(c:vec3f)->f32{return min(c.r,min(c.g,c.b));}
fn max4(v:vec4f)->f32{return max(max(v.x,v.y),max(v.z,v.w));}
fn min4(v:vec4f)->f32{return min(min(v.x,v.y),min(v.z,v.w));}
fn pixel(p:vec2i)->vec3f{
    return textureLoad(scene,clamp(p,vec2i(0),vec2i(textureDimensions(scene))-1),0).rgb;
}
fn linear_depth(p:vec2i)->f32{
    let sample=textureLoad(depth,clamp(p,vec2i(0),vec2i(textureDimensions(depth))-1),0).r;
    // Keep reverse-Z in its native range. Subtracting two far-plane-sized
    // values first loses depth precision and draws stripes on distant slopes.
    let distanceFromFar=select(1.0-sample,sample,u.camera.z>0.5);
    return u.camera.x/max(u.camera.x+distanceFromFar*(u.camera.y-u.camera.x),0.0001);
}

// Comic's distance/luma/saturation strength curve and mesh curvature test.
fn strength_curve(fade:vec3f,x:f32)->f32{
    return smoothstep(0.0,1.0-fade.z,x+(0.2-1.2*fade.x))*
           smoothstep(0.0,1.0-fade.z,1.0-x+(1.2*fade.y-1.0));
}
fn mesh_edge(c:f32,a:vec4f,b:vec4f)->f32{
    let span=max(max4(a),max4(b))-min(min4(a),min4(b))+0.00001;
    let cardinal=abs(vec4f(a.x,b.x,a.z,b.z)-c)/span;
    let diagonal=abs(vec4f(a.y,b.y,a.w,b.w)-c)/span;
    return max(max(abs(cardinal.x-cardinal.y),abs(cardinal.z-cardinal.w)),
               max(abs(diagonal.x-diagonal.y),abs(diagonal.z-diagonal.w)));
}
fn comic(p:vec2i,c:vec3f)->vec3f{
    let luma=vec3f(0.2126,0.7151,0.0721);
    let center=dot(c,luma);
    let width=i32(u.settings.z);
    var a:array<vec3f,4>;var b:array<vec3f,4>;
    a[0]=pixel(p+vec2i(0,-width)); a[1]=pixel(p+vec2i(width,-width));
    a[2]=pixel(p+vec2i(width,0)); a[3]=pixel(p+vec2i(width,width));
    b[0]=pixel(p+vec2i(0,width)); b[1]=pixel(p+vec2i(-width,width));
    b[2]=pixel(p+vec2i(-width,0)); b[3]=pixel(p+vec2i(-width,-width));
    let la=vec4f(dot(a[0],luma),dot(a[1],luma),dot(a[2],luma),dot(a[3],luma));
    let lb=vec4f(dot(b[0],luma),dot(b[1],luma),dot(b[2],luma),dot(b[3],luma));
    // DrawnShade: single-pass Sobel color edges, value-difference chroma edges.
    let colorEdge=clamp(max(dot(la,vec4f(0,-1,-2,-1))+dot(lb,vec4f(0,1,2,1)),
        dot(la,vec4f(2,1,0,-1))+dot(lb,vec4f(-2,-1,0,1))),0.0,1.0);
    let ca=vec4f(hi(a[0]-la.x),hi(a[1]-la.y),hi(a[2]-la.z),hi(a[3]-la.w));
    let cb=vec4f(hi(b[0]-lb.x),hi(b[1]-lb.y),hi(b[2]-lb.z),hi(b[3]-lb.w));
    let chromaEdge=clamp(dot(abs(ca-cb),vec4f(1))*(1.0-dot(c-center,luma)),0.0,1.0)*0.5;
    let d=linear_depth(p); var mesh=0.0;
    var firstA=vec4f(0); var firstB=vec4f(0);
    for(var i=1;i<=3;i++){
        if(f32(i)>u.settings.z){break;}
        let da=vec4f(linear_depth(p+vec2i(0,-i)),linear_depth(p+vec2i(i,-i)),
            linear_depth(p+vec2i(i,0)),linear_depth(p+vec2i(i,i)));
        let db=vec4f(linear_depth(p+vec2i(0,i)),linear_depth(p+vec2i(-i,i)),
            linear_depth(p+vec2i(-i,0)),linear_depth(p+vec2i(-i,-i)));
        if(i==width){firstA=da;firstB=db;}
        mesh=max(mesh,mesh_edge(d,da,db));
    }
    let pix=f32(width)/vec2f(textureDimensions(scene));
    let normal=normalize(cross(vec3f(0,pix.y,d-firstA.x),vec3f(-pix.x,0,d-firstA.z)));
    let outline=1.0-clamp(normal.z*0.5+0.5,0.0,1.0);
    // Values from DrawnShade.ini. Width is adjustable for the actual render resolution.
    var edges=vec4f(colorEdge,chromaEdge,outline,pow(clamp(mesh,0.0,1.0),1.15)*2.5*u.settings.w);
    edges*=vec4f(strength_curve(vec3f(0,0.1,0.8),d),strength_curve(vec3f(0,0.5,0.8),d),
        strength_curve(vec3f(0,1,0.8),d),strength_curve(vec3f(-1,0.1,0.8),d));
    edges*=min(strength_curve(vec3f(-0.855,1,0.8),center),strength_curve(vec3f(0,1,0.8),hi(c)-lo(c)));
    return sat(mix(c,vec3f(0),max4(edges)));
}

// SweetFX LiftGammaGain, Tonemap, Vibrance, Curves, DPX and Sepia.
fn lift_gamma_gain(c:vec3f,lift:vec3f,gamma:vec3f,gain:vec3f)->vec3f{
    return sat(pow(abs(sat(c*(1.5-0.5*lift)+0.5*lift-0.5)*gain),1.0/gamma));
}
fn tonemap(input:vec3f,gamma:f32,exposure:f32,saturation:f32,bleach:f32,defog:f32,fog:vec3f)->vec3f{
    var c=pow(sat(input-defog*fog)*exp2(exposure),vec3f(gamma));
    let lum=dot(c,vec3f(0.2126,0.7152,0.0722));
    let l=clamp(10.0*(lum-0.45),0.0,1.0);
    let a=bleach*c;
    let bleached=mix(2.0*c*lum,1.0-2.0*(1.0-lum)*(1.0-c),l);
    c+=(1.0-a)*(a*bleached);
    let diff=(c-dot(c,vec3f(1.0/3.0)))*saturation;
    return sat((c+diff)/(1.0+diff));
}
fn vibrance(c:vec3f,v:f32,balance:vec3f)->vec3f{
    let l=dot(c,vec3f(0.212656,0.715158,0.072186));
    let amount=v*balance;
    return sat(mix(vec3f(l),c,1.0+amount*(1.0-sign(amount)*(hi(c)-lo(c)))));
}
fn curves(c:vec3f,contrast:f32,formula:i32)->vec3f{
    let l=dot(c,vec3f(0.2126,0.7152,0.0722));var x=l;
    // SweetFX 1.x formulas are one-based: 2 = abs split, 3 = smoothstep.
    if(formula==2){let v=l-0.5;x=v/(0.5+abs(v))+0.5;}
    else{x=l*l*(3.0-2.0*l);}
    return sat(c+(x-l)*contrast);
}
fn sepia(c:vec3f,tone:vec3f,grey:f32,power:f32)->vec3f{
    let l=dot(c,vec3f(0.2126,0.7152,0.0722));
    return sat(mix(c,mix(c,vec3f(l),grey)*tone,power));
}
fn dpx(input:vec3f)->vec3f{
    let curve=vec3f(8);let center=vec3f(0.33,0.36,0.34);
    let t=1.0/(1.0+exp(curve*0.5));
    let b=(1.0/(1.0+exp(-curve*(input-center)))-t)/(1.0-2.0*t);
    let value=max(hi(b),0.00001);
    let rgb=pow(abs(b/value),vec3f(1.0/1.1))*value;
    let xyz=vec3f(dot(vec3f(0.50030338,0.33809757,0.16458978),rgb),
        dot(vec3f(0.25796889,0.67619526,0.06583585),rgb),
        dot(vec3f(0.02345179,0.11269927,0.86683967),rgb));
    let out=vec3f(dot(vec3f(2.67147117,-1.26723606,-0.41099560),xyz),
        dot(vec3f(-1.02510703,1.98409116,0.04395025),xyz),
        dot(vec3f(0.06100095,-0.22367075,1.15902104),xyz));
    return sat(mix(input,out,0.2));
}
fn grade(input:vec3f,style:i32)->vec3f{
    var c=input;
    if(style==2){
        // SwordDeath: Enhanced Colors and Lighting v2, color stages only.
        c=lift_gamma_gain(c,vec3f(0.965,0.960,0.970),vec3f(1.03,1,1.025),vec3f(1.010,1,0.999));
        c=tonemap(c,1.16,0,-0.10,0,0,vec3f(0));
        c=vibrance(c,0.16,vec3f(1.20,1.20,1.22));
        c=curves(c,-0.16,2);
    }else if(style==3){
        // SwordDeath: Better Lighting, color stages only.
        c=sat((c-2.0/255.0)*(255.0/253.0));
        c=dpx(c);
        c=lift_gamma_gain(c,vec3f(0.955,0.95,0.96),vec3f(0.987,0.98,0.97),vec3f(0.99,0.99,1));
        c=curves(c,0.2,3);
        c=sepia(c,vec3f(1,1,1.25),0,0.05);
    }else if(style==4){
        // Magiike: ChromeFX. Bloom, Gaussian blur, grain and vignette omitted.
        c=tonemap(c,0.870,-0.053,0.145,0.165,0.005,vec3f(0,0,2.55));
        c=vibrance(c,-0.03,vec3f(1));
        c=sepia(c,vec3f(1.88,1.25,0.44),0.15,0.05);
    }
    return c;
}
@fragment fn fragment_main(@builtin(position) position:vec4f)->@location(0) vec4f{
    let p=vec2i(position.xy);let original=pixel(p);let style=i32(u.settings.x);
    var result=original;
    if(style==1){result=comic(p,original);}else{result=grade(original,style);}
    return vec4f(mix(original,result,u.settings.y),1);
}
