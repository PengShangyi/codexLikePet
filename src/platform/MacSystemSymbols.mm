#include "platform/SystemSymbols.h"

#include <QColor>
#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QtGlobal>

#import <AppKit/AppKit.h>

namespace SystemSymbols {

QIcon icon(const QString &symbolName, int pointSize, const QColor &tint)
{
    if (pointSize <= 0) return {};
    if (@available(macOS 11.0, *)) {
        NSImage *symbol = [NSImage imageWithSystemSymbolName:symbolName.toNSString()
                                   accessibilityDescription:nil];
        if (!symbol) return {};

        NSImageSymbolConfiguration *configuration =
            [NSImageSymbolConfiguration configurationWithPointSize:pointSize
                                                           weight:NSFontWeightRegular];
        NSImage *configured = [symbol imageWithSymbolConfiguration:configuration];
        if (!configured) configured = symbol;

        // Render at 2x and let QIcon scale down. SF Symbols are vector, but an
        // NSImage has to be rasterized to cross into Qt, and the settings window can
        // move between a Retina and a non-Retina display while open.
        constexpr qreal scale = 2.0;
        const NSSize size = configured.size;
        if (size.width <= 0 || size.height <= 0) return {};
        const QSize target(qRound(size.width * scale), qRound(size.height * scale));

        NSBitmapImageRep *rep = [[NSBitmapImageRep alloc]
            initWithBitmapDataPlanes:nullptr
                          pixelsWide:target.width()
                          pixelsHigh:target.height()
                       bitsPerSample:8
                     samplesPerPixel:4
                            hasAlpha:YES
                            isPlanar:NO
                      colorSpaceName:NSDeviceRGBColorSpace
                         bytesPerRow:target.width() * 4
                        bitsPerPixel:32];
        if (!rep) return {};

        [NSGraphicsContext saveGraphicsState];
        NSGraphicsContext.currentContext =
            [NSGraphicsContext graphicsContextWithBitmapImageRep:rep];
        [configured drawInRect:NSMakeRect(0, 0, target.width(), target.height())
                      fromRect:NSZeroRect
                     operation:NSCompositingOperationSourceOver
                      fraction:1.0];
        [NSGraphicsContext restoreGraphicsState];

        QImage image(rep.bitmapData, target.width(), target.height(),
                     static_cast<qsizetype>(rep.bytesPerRow), QImage::Format_RGBA8888);
        // rep owns the pixels and is autoreleased, so copy before it goes away.
        QPixmap pixmap = QPixmap::fromImage(image.copy());
        if (pixmap.isNull()) return {};

        // Symbols arrive as black-on-transparent template art. Tint by keeping the
        // alpha and replacing the color, which is what makes one symbol usable in
        // both the light and dark palettes.
        QPainter painter(&pixmap);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(pixmap.rect(), tint);
        painter.end();
        pixmap.setDevicePixelRatio(scale);
        return QIcon(pixmap);
    }
    Q_UNUSED(symbolName);
    Q_UNUSED(tint);
    return {};
}

}  // namespace SystemSymbols
