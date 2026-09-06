import SwiftUI

/// Dialogs retain their content's natural height until they reach the available
/// screen height; long translations can then scroll without clipping actions.
struct HeightLimitedLocalizationView<Content: View>: View {
    let width: CGFloat
    let maximumHeight: CGFloat
    @ViewBuilder let content: () -> Content
    @State private var contentHeight: CGFloat = 420

    var body: some View {
        ScrollView(.vertical) {
            content()
                .background(GeometryReader { geometry in
                    Color.clear.preference(key: LocalizationContentHeight.self,
                                           value: geometry.size.height)
                })
        }
        .frame(width: width, height: min(contentHeight, maximumHeight))
        .onPreferenceChange(LocalizationContentHeight.self) { height in
            if height > 1 { contentHeight = height }
        }
    }
}

private struct LocalizationContentHeight: PreferenceKey {
    static let defaultValue: CGFloat = 0
    static func reduce(value: inout CGFloat, nextValue: () -> CGFloat) {
        value = max(value, nextValue())
    }
}

/// Keep each selector on one line and distribute spare width across its buttons.
/// The panel measures the widest translated row before laying out these controls.
struct LocalizedOptionsLayout: Layout {
    var spacing: CGFloat = 6

    private func sizes(_ subviews: Subviews, width: CGFloat?) -> [CGSize] {
        let ideal = subviews.map { $0.sizeThatFits(.unspecified) }
        let minimum = ideal.reduce(0) { $0 + $1.width } + spacing * CGFloat(max(0, ideal.count - 1))
        let extra = max(0, (width ?? minimum) - minimum) / CGFloat(max(1, ideal.count))
        return ideal.map { CGSize(width: $0.width + extra, height: $0.height) }
    }

    func sizeThatFits(proposal: ProposedViewSize, subviews: Subviews, cache: inout ()) -> CGSize {
        let items = sizes(subviews, width: proposal.width)
        return CGSize(width: items.reduce(0) { $0 + $1.width } + spacing * CGFloat(max(0, items.count - 1)),
                      height: items.map(\.height).max() ?? 0)
    }

    func placeSubviews(in bounds: CGRect, proposal: ProposedViewSize, subviews: Subviews, cache: inout ()) {
        var x = bounds.minX
        for (view, size) in zip(subviews, sizes(subviews, width: bounds.width)) {
            view.place(at: CGPoint(x: x, y: bounds.minY), anchor: .topLeading,
                       proposal: ProposedViewSize(size))
            x += size.width + spacing
        }
    }
}
