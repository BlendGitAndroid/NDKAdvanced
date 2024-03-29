precision mediump float;

uniform sampler2D vTexture;
uniform int vChangeType;
uniform vec3 vChangeColor;
uniform int vIsHalf;
uniform float uXY;

varying vec4 gPosition; //最终显示的位置

varying vec2 aCoordinate;   // 纹理坐标系
varying vec4 aPos;  // 世界坐标系

void modifyColor(vec4 color){
    color.r=max(min(color.r, 1.0), 0.0);
    color.g=max(min(color.g, 1.0), 0.0);
    color.b=max(min(color.b, 1.0), 0.0);
    color.a=max(min(color.a, 1.0), 0.0);
}

void main(){
    // 开始渲染纹理坐标系
    vec4 nColor=texture2D(vTexture, aCoordinate);
    // aPos.x>0.0表示世界坐标系中心点的右边，也就是右半边
    // vIsHalf表示是否处理一半，如果vIsHalf为0，则vIsHalf==0始终为true，就全部处理
    // 如果vIsHalf==1，那只有aPos.x>0.0的点才处理，也就是右半边处理
    if (aPos.x>0.0||vIsHalf==0){
        if (vChangeType==1){
            float c=nColor.r*vChangeColor.r+nColor.g*vChangeColor.g+nColor.b*vChangeColor.b;
            gl_FragColor=vec4(c, c, c, nColor.a);
        } else if (vChangeType==2){
            vec4 deltaColor=nColor+vec4(vChangeColor, 0.0);
            modifyColor(deltaColor);
            gl_FragColor=deltaColor;
        } else if (vChangeType==3){ // shader采样了周边12个点的颜色加上当前点的颜色，就是13个了，除以13得到均值。
            // 同理,这里的颜色值,也是为了计算距离的
            nColor+=texture2D(vTexture, vec2(aCoordinate.x-vChangeColor.r, aCoordinate.y-vChangeColor.r));
            nColor+=texture2D(vTexture, vec2(aCoordinate.x-vChangeColor.r, aCoordinate.y+vChangeColor.r));
            nColor+=texture2D(vTexture, vec2(aCoordinate.x+vChangeColor.r, aCoordinate.y-vChangeColor.r));
            nColor+=texture2D(vTexture, vec2(aCoordinate.x+vChangeColor.r, aCoordinate.y+vChangeColor.r));
            nColor+=texture2D(vTexture, vec2(aCoordinate.x-vChangeColor.g, aCoordinate.y-vChangeColor.g));
            nColor+=texture2D(vTexture, vec2(aCoordinate.x-vChangeColor.g, aCoordinate.y+vChangeColor.g));
            nColor+=texture2D(vTexture, vec2(aCoordinate.x+vChangeColor.g, aCoordinate.y-vChangeColor.g));
            nColor+=texture2D(vTexture, vec2(aCoordinate.x+vChangeColor.g, aCoordinate.y+vChangeColor.g));
            nColor+=texture2D(vTexture, vec2(aCoordinate.x-vChangeColor.b, aCoordinate.y-vChangeColor.b));
            nColor+=texture2D(vTexture, vec2(aCoordinate.x-vChangeColor.b, aCoordinate.y+vChangeColor.b));
            nColor+=texture2D(vTexture, vec2(aCoordinate.x+vChangeColor.b, aCoordinate.y-vChangeColor.b));
            nColor+=texture2D(vTexture, vec2(aCoordinate.x+vChangeColor.b, aCoordinate.y+vChangeColor.b));
            nColor/=13.0;
            gl_FragColor=nColor;
        } else if (vChangeType==4){ //放大镜的效果,在放大的半径内就放大，不在就正常显示
            // 这里的颜色值,就是为了计算距离的,vec2(vChangeColor.r, vChangeColor.g)就是世界坐标系,就是中心点
            float dis=distance(vec2(gPosition.x, gPosition.y/uXY), vec2(vChangeColor.r, vChangeColor.g));
            if (dis<vChangeColor.b){    // vChangeColor.b就是半径
                // 每个像素在采样时得到的纹理坐标都会变成原来的一半，同时在水平和竖直方向上分别向右和向下偏移了0.25个纹理单元
                // 始终注意,纹理的单位是0-1,图像的位置在中间,就是(0.5,0.5),除以2,就是0.25,所以要偏移0.25
                // aCoordinate的0点在左上角,所以要加0.25,才能偏移0.25
                nColor=texture2D(vTexture, vec2(aCoordinate.x/2.0+0.25, aCoordinate.y/2.0+0.25));
            }
            gl_FragColor=nColor;
        } else {
            gl_FragColor=nColor;
        }
    } else {
        gl_FragColor=nColor;
    }
}