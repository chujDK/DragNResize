# Drag & Resize in windows with mouse + alt

写完之后才发现早就有别人写过了。。

a toy project, does the same thing like [AltSnap](https://github.com/RamonUnch/AltSnap)

我怎么就没早点想到先去查一下呢。不过作为 win32 api 使用练习也还行

想玩玩的话可以在 reslease 中下载我编译好的，也可以自己拿 Visual Studio 编译（选择 cmake 项目就可以）

比较重要的是，由于我没处理高 dpi 的问题，所以如果要使用这个程序，你需要设置程序的高 DIP 缩放行为为应用程序。通过右键程序，打开属性，在兼容性一栏中，点击更改高 DPI 设置，勾选替代高 DPI 缩放行为，并修改为应用程序。
